#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "servtp3.h"
#include <ifaddrs.h>
#include <netdb.h>
#include <readline/readline.h>

#ifndef BROADCAST_IP
#define BROADCAST_IP "192.168.88.255"
#endif

#ifdef TRACE
    #define DEBUG 1
#else
    #define DEBUG 0
#endif

#define PORT 9998
#define LBUF 512
#define IP_BASE ((192U << 24) | (168 << 16) | (88 << 8))

// ==========================================
// VARIABLES GLOBALES ET UTILITAIRES
// ==========================================

bool serveur_actif = false;
struct sockaddr_in SockConf;
struct elt * liste_contacts = NULL;
pthread_mutex_t mutex_contacts = PTHREAD_MUTEX_INITIALIZER;

char * addrip(unsigned long A) {
    static char b[16];
    sprintf(b,"%u.%u.%u.%u", (unsigned int)(A>>24&0xFF), (unsigned int)(A>>16&0xFF),
           (unsigned int)(A>>8&0xFF), (unsigned int)(A&0xFF));
    return b;
}

// ==========================================
// GESTION DE LA LISTE CHAÎNÉE (CONTACTS)
// ==========================================

void viderListeContacts() {
    pthread_mutex_lock(&mutex_contacts);
    struct elt *courant = liste_contacts;
    while (courant != NULL) {
        struct elt *suivant = courant->next;
        free(courant);
        courant = suivant;
    }
    liste_contacts = NULL;
    pthread_mutex_unlock(&mutex_contacts);
}

void ajouteElt(char * pseudo, char * adip) {
    pthread_mutex_lock(&mutex_contacts);
    struct elt * courant = liste_contacts;
    struct elt * precedent = NULL;

    while (courant != NULL) {
        if (strcmp(courant->adip, adip) == 0) {
            if (precedent == NULL) liste_contacts = courant->next;
            else precedent->next = courant->next;
            free(courant);
            break;
        }
        precedent = courant;
        courant = courant->next;
    }

    struct elt * nouveau = malloc(sizeof(struct elt));
    strncpy(nouveau->nom, pseudo, LPSEUDO);
    nouveau->nom[LPSEUDO] = '\0';
    strncpy(nouveau->adip, adip, 15);
    nouveau->adip[15] = '\0';

    if (liste_contacts == NULL || strcmp(liste_contacts->nom, pseudo) > 0) {
        nouveau->next = liste_contacts;
        liste_contacts = nouveau;
    } else {
        courant = liste_contacts;
        while (courant->next != NULL && strcmp(courant->next->nom, pseudo) < 0) {
            courant = courant->next;
        }
        nouveau->next = courant->next;
        courant->next = nouveau;
    }
    pthread_mutex_unlock(&mutex_contacts);
}

void supprimeElt(char * adip) {
    pthread_mutex_lock(&mutex_contacts);
    struct elt * courant = liste_contacts;
    struct elt * precedent = NULL;

    while (courant != NULL) {
        if (strcmp(courant->adip, adip) == 0) {
            if (precedent == NULL) liste_contacts = courant->next;
            else precedent->next = courant->next;
            free(courant);
            break;
        }
        precedent = courant;
        courant = courant->next;
    }
    pthread_mutex_unlock(&mutex_contacts);
}

void listeElts(void) {
    pthread_mutex_lock(&mutex_contacts);
    struct elt * courant = liste_contacts;
    if (courant == NULL) {
        printf("Aucun contact en ligne.\n");
    } else {
        while (courant != NULL) {
            printf("%s : %s\n", courant->adip, courant->nom);
            courant = courant->next;
        }
    }
    pthread_mutex_unlock(&mutex_contacts);
}

/// Cherche l'IP d'un pseudo dans la liste. Retourne true si trouvé.
bool trouverIpParPseudo(char * pseudo, char * ip_dest) {
    bool trouve = false;
    ip_dest[0] = '\0';
    pthread_mutex_lock(&mutex_contacts);
    struct elt *courant = liste_contacts;
    while(courant != NULL) {
        if (strcmp(pseudo, courant->nom) == 0) {
            strcpy(ip_dest, courant->adip);
            trouve = true;
            break;
        }
        courant = courant->next;
    }
    pthread_mutex_unlock(&mutex_contacts);
    return trouve;
}

// ==========================================
// RESEAU : SERVEUR UDP & COMMANDES MESSAGES
// ==========================================

int initServer(struct sockaddr_in *socketConf) {
    memset(socketConf, 0, sizeof(struct sockaddr_in));
    int sid;
    if ((sid = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        perror("socket");
        return -1;
    }
    socketConf->sin_family = AF_INET;
    socketConf->sin_port = htons(PORT);
    socketConf->sin_addr.s_addr = htonl(INADDR_ANY);

    int opt = 1;
    setsockopt(sid, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(sid, (struct sockaddr *) socketConf, sizeof(*socketConf)) == -1) {
        perror("bind");
        return -1;
    }

    int broadcast = 1;
    if (setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1) {
        perror("setsockopt");
        return -1;
    }
    return sid;
}

int sendBroadcastConnection(int sid, char Code, char* message) {
    char buf[LBUF];
    sprintf(buf, "%cBEUIP%s", Code, message);

    // Priorité à la macro (Pour le professeur)
    struct sockaddr_in broadcastSock = {0};
    broadcastSock.sin_family = AF_INET;
    broadcastSock.sin_port = htons(PORT);
    inet_pton(AF_INET, BROADCAST_IP, &broadcastSock.sin_addr);

    if (sendto(sid, buf, strlen(buf), 0, (struct sockaddr *) &broadcastSock, sizeof(broadcastSock)) == -1) {
        perror("sendto macro");
    } else if (DEBUG) {
        printf("Broadcast envoyé sur l'IP par défaut (%s)\n", BROADCAST_IP);
    }

    // Détection dynamique
    struct ifaddrs *ifap, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifap) != -1) {
        for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == NULL) continue;
            if (ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_broadaddr != NULL) {
                int s = getnameinfo(ifa->ifa_broadaddr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
                if (s == 0 && strncmp(host, "127.", 4) != 0) {
                    struct sockaddr_in broadcastSockDyn = {0};
                    broadcastSockDyn.sin_family = AF_INET;
                    broadcastSockDyn.sin_port = htons(PORT);
                    inet_pton(AF_INET, host, &broadcastSockDyn.sin_addr);

                    if (sendto(sid, buf, strlen(buf), 0, (struct sockaddr *) &broadcastSockDyn, sizeof(broadcastSockDyn)) == -1) {
                        perror("sendto dynamique");
                    } else if (DEBUG) {
                        printf("Broadcast envoyé sur l'interface %s (%s)\n", ifa->ifa_name, host);
                    }
                }
            }
        }
        freeifaddrs(ifap);
    }
    return 0;
}

int sendAck(int sid, struct sockaddr_in *clientSock, char* serverPseudo) {
    char ackBuf[LBUF];
    sprintf(ackBuf, "2BEUIP%s", serverPseudo);

    if (sendto(sid, ackBuf, strlen(ackBuf), 0, (struct sockaddr *) clientSock, sizeof(*clientSock)) == -1) {
        perror("sendto ack");
        return -1;
    }
    if (DEBUG) printf("Ack envoyé à %s!\n", addrip(ntohl(clientSock->sin_addr.s_addr)));
    return 0;
}

void * serveur_udp(void * p) {
    char pseudo_local[LPSEUDO+1];
    strncpy(pseudo_local, (char *) p, LPSEUDO);
    pseudo_local[LPSEUDO] = '\0';

    int n;
    struct sockaddr_in Sock;
    socklen_t ls;
    char buf[LBUF];

    int sid = initServer(&SockConf);
    if (sid == -1) {
        serveur_actif = false;
        pthread_exit(NULL);
    }

    if (sendBroadcastConnection(sid, '1', pseudo_local) == -1) {
        serveur_actif = false;
        pthread_exit(NULL);
    }
    serveur_actif = true;

    while (serveur_actif) {
        ls = sizeof(Sock);
        n = recvfrom(sid, (void*)buf, LBUF, MSG_DONTWAIT, (struct sockaddr *)&Sock, &ls);

        if (n > 0) {
            buf[n] = '\0';
            char entete = buf[0];
            char *ip_expediteur_chaine = inet_ntoa(Sock.sin_addr);
            char message[LBUF] = "";

            if (n > 6) strcpy(message, buf + 6);

            if (strncmp(buf + 1, "BEUIP", 5) == 0) {
                switch (entete) {
                    case '1':
                    case '2':
                        ajouteElt(message, ip_expediteur_chaine);
                        if (DEBUG) printf("Utilisateur mis à jour : %s (%s)\n", message, ip_expediteur_chaine);
                        if (entete == '1') sendAck(sid, &Sock, pseudo_local);
                        break;
                    case '9': {
                        char expediteur[LPSEUDO+1] = "inconnu";
                        pthread_mutex_lock(&mutex_contacts);
                        struct elt *courant = liste_contacts;
                        while(courant != NULL) {
                            if (strcmp(courant->adip, ip_expediteur_chaine) == 0) {
                                strcpy(expediteur, courant->nom);
                                break;
                            }
                            courant = courant->next;
                        }
                        pthread_mutex_unlock(&mutex_contacts);

                        printf("\r\033[K");
                        printf("[Message de %s] : %s\n", expediteur, message);
                        rl_on_new_line();
                        rl_redisplay();
                        fflush(stdout);
                        break;
                    }
                    case '0':
                        supprimeElt(ip_expediteur_chaine);
                        break;
                }
            }
        }
        usleep(10000);
    }
    close(sid);
    viderListeContacts();
    return NULL;
}

void commande(char octet1, char * message, char * pseudo) {
    int sid = socket(AF_INET, SOCK_DGRAM, 0);
    int broadcast = 1;
    setsockopt(sid, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in target_addr = {0};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(PORT);

    switch (octet1) {
        case '3':
            listeElts();
            break;
        case '4': {
            char ip_dest[16];
            if (trouverIpParPseudo(pseudo, ip_dest)) {
                inet_pton(AF_INET, ip_dest, &target_addr.sin_addr);
                char sendbuf[LBUF];
                int len = sprintf(sendbuf, "9BEUIP%s", message);
                sendto(sid, sendbuf, len, 0, (struct sockaddr *) &target_addr, sizeof(target_addr));
                printf("Message transféré à %s.\n", pseudo);
            } else {
                printf("Utilisateur %s inconnu.\n", pseudo);
            }
            break;
        }
        case '5':
        case '0':
            if (octet1 == '5') {
                sendBroadcastConnection(sid, '9', message);
                printf("Message transféré à tout le monde.\n");
            } else {
                sendBroadcastConnection(sid, '0', "quit");
                serveur_actif = false;
                printf("Message de sortie envoyé.\n");
            }
            break;
    }
    close(sid);
}

// ==========================================
// RESEAU : SERVEUR TCP & FICHIERS
// ==========================================

void traiterDemandeListeTCP(int fd, char* rep) {
    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
        execlp("ls", "ls", "-l", rep, NULL);
        perror("execlp ls");
        exit(-1);
    } else if (pid > 0) {
        close(fd);
    } else {
        perror("fork ls");
        close(fd);
    }
}

void traiterDemandeFichierTCP(int fd, char* rep) {
    char nomfic[256];
    int i = 0;
    char c;

    while (read(fd, &c, 1) > 0 && c != '\n' && i < 255) {
        nomfic[i++] = c;
    }
    nomfic[i] = '\0';

    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        char chemin[512];
        snprintf(chemin, sizeof(chemin), "%s/%s", rep, nomfic);

        if (access(chemin, R_OK) == -1) {
            char *err = "ERREUR: Fichier introuvable ou inaccessible sur le serveur.\n";
            write(fd, err, strlen(err));
            close(fd);
            return;
        }
        execlp("cat", "cat", chemin, NULL);
        perror("execlp cat");
        exit(-1);
    } else if (pid > 0) {
        close(fd);
    } else {
        perror("fork get");
        close(fd);
    }
}

void envoiContenu(int fd, char* rep) {
    char octet;
    if (read(fd, &octet, 1) <= 0) {
        close(fd);
        return;
    }

    if (octet == 'L') {
        traiterDemandeListeTCP(fd, rep);
    } else if (octet == 'F') {
        traiterDemandeFichierTCP(fd, rep);
    } else {
        printf("Commande TCP non reconnue : %c\n", octet);
        close(fd);
    }
}

void * serveur_tcp(void * rep) {
    char * repertoire = (char *) rep;
    int sid_tcp;
    struct sockaddr_in TCP_Conf = {0};

    if ((sid_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket tcp");
        pthread_exit(NULL);
    }

    int opt = 1;
    setsockopt(sid_tcp, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    TCP_Conf.sin_family = AF_INET;
    TCP_Conf.sin_port = htons(PORT);
    TCP_Conf.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sid_tcp, (struct sockaddr *) &TCP_Conf, sizeof(TCP_Conf)) == -1) {
        perror("bind tcp");
        close(sid_tcp);
        pthread_exit(NULL);
    }

    if (listen(sid_tcp, 5) == -1) {
        perror("listen tcp");
        close(sid_tcp);
        pthread_exit(NULL);
    }

    while (serveur_actif) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int fd_client = accept(sid_tcp, (struct sockaddr *)&client_addr, &client_len);

        if (!serveur_actif) {
            if (fd_client >= 0) close(fd_client);
            break;
        }
        if (fd_client < 0) continue;

        envoiContenu(fd_client, repertoire);
    }

    close(sid_tcp);
    return NULL;
}

void demandeListe(char * pseudo) {
    char ip_dest[16];
    if (!trouverIpParPseudo(pseudo, ip_dest)) {
        printf("Utilisateur %s introuvable.\n", pseudo);
        return;
    }

    int sid = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip_dest, &server_addr.sin_addr);

    if (connect(sid, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect tcp (ls)");
        close(sid);
        return;
    }

    write(sid, "L", 1);
    char buf[512];
    int n;

    printf("--- Fichiers de %s ---\n", pseudo);
    while ((n = read(sid, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("------------------------\n");
    close(sid);
}

void demandeFichier(char * pseudo, char * nomfic) {
    char ip_dest[16];
    if (!trouverIpParPseudo(pseudo, ip_dest)) {
        printf("Utilisateur %s introuvable.\n", pseudo);
        return;
    }

    int sid = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip_dest, &server_addr.sin_addr);

    if (connect(sid, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect tcp (get)");
        close(sid);
        return;
    }

    char requete[512];
    snprintf(requete, sizeof(requete), "F%s\n", nomfic);
    write(sid, requete, strlen(requete));

    char chemin_local[512];
    snprintf(chemin_local, sizeof(chemin_local), "reppub/%s", nomfic);

    if (access(chemin_local, F_OK) == 0) {
        printf("Attention : le fichier '%s' existe déjà. Voulez-vous l'écraser ? (o/n) : ", nomfic);
        char reponse = getchar();
        int c;
        while ((c = getchar()) != '\n' && c != EOF);

        if (reponse != 'o' && reponse != 'O') {
            printf("Téléchargement annulé.\n");
            close(sid);
            return;
        }
    }

    char buf[1024];
    int n = read(sid, buf, sizeof(buf) - 1);

    if (n <= 0) {
        printf("Erreur de communication avec le serveur (ou fichier vide).\n");
        close(sid);
        return;
    }
    buf[n] = '\0';

    if (strncmp(buf, "ERREUR", 6) == 0) {
        printf("%s", buf);
        close(sid);
        return;
    }

    FILE * f = fopen(chemin_local, "w");
    if (!f) {
        perror("fopen local");
        close(sid);
        return;
    }

    fwrite(buf, 1, n, f);
    while ((n = read(sid, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, n, f);
    }

    fclose(f);
    close(sid);
    printf("Fichier '%s' téléchargé avec succès depuis %s.\n", nomfic, pseudo);
}