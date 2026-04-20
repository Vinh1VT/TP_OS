#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include "gescom.h"
#include "../tp3/servtp3.h"
#include <readline/history.h>
#include <sys/socket.h>
#include <arpa/inet.h>

static Commande tableauCommande[NBMAXC];
static int NBCom = 0;
static pthread_t thread_udp;
static pthread_t thread_tcp; // Ajout du thread TCP pour l'étape 3

char ** Mots = NULL;
int NMots = 0;

/// Construit un message à partir des argv séparés.
void construireMessage(int argc, char* argv[], int index_debut, char* messageOut) {
    messageOut[0] = '\0';
    for (int i = index_debut; i < argc; i++) {
        strcat(messageOut, argv[i]);
        if (i < argc - 1) {
            strcat(messageOut, " ");
        }
    }
}

/// COMMANDES
int Sortie(int argc,char* argv[]){exit(0);}
int Vers(int argc, char* argv[]) {
    printf("biceps version %.2f\n",VERSION);
    return 0;
}
int changeDirectory(int argc,char* argv[]) {
    if (argv[1] == NULL) {
        perror("Pas assez d'arguments. Utilisation: cd dossier");
        return 1;
    }
    if (chdir(argv[1]) == -1) {
        perror("cd");
        return 1;
    }
    return 0;
}
int printWorkingDirectory(int argc,char* argv[]) {
    char * path = getcwd(NULL,0);
    if (path != NULL) {
        printf("%s\n",path);
        free(path);
    }else {
        perror("pwd");
    }
    return 0;
}

/// Démarrage et extinction du serveur BEUIP (Étape 1.1 et 3.1)
int Beuip(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Utilisation: beuip <commande> \n");
        return 1;
    }

    if (strcmp(argv[1],"start")==0) {
        if (argc < 3) {
            printf("Utilisation: beuip start <pseudo> \n");
            return 1;
        }
        if (serveur_actif) {
            printf("Le serveur est déjà actif.\n");
            return 1;
        }

        if (pthread_create(&thread_udp, NULL, serveur_udp, argv[2]) != 0) {
            perror("pthread_create udp");
            return 1;
        }

        if (pthread_create(&thread_tcp, NULL, serveur_tcp, "reppub") != 0) {
            perror("pthread_create tcp");
            return 1;
        }

        printf("Serveurs UDP et TCP démarrés avec le pseudo '%s'\n", argv[2]);

    } else if (strcmp(argv[1],"stop")==0) {
        if (!serveur_actif) {
            printf("Aucun serveur en cours d'exécution.\n");
            return 1;
        }

        commande('0', NULL, NULL);

        pthread_join(thread_udp, NULL);

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9998);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(sock, (struct sockaddr *)&addr, sizeof(addr));
        close(sock);

        pthread_join(thread_tcp, NULL);

        printf("Serveurs arrêtés avec succès.\n");

    }
    else if (strcmp(argv[1], "list") == 0) {
        if (!serveur_actif) { printf("Serveur inactif.\n"); return 1; }
        commande('3', NULL, NULL);
    }
    else if (strcmp(argv[1], "message") == 0) {
        if (!serveur_actif) { printf("Serveur inactif.\n"); return 1; }
        if (argc < 4) {
            printf("Utilisation: beuip message <pseudo|all> <message>\n");
            return 1;
        }
        char message[1024];
        construireMessage(argc, argv, 3, message);

        if (strcmp(argv[2], "all") == 0) {
            commande('5', message, NULL); // Message à tout le monde
        } else {
            commande('4', message, argv[2]); // Message à une personne
        }
    }
    else if (strcmp(argv[1], "ls") == 0) {
        if (argc < 3) {
            printf("Utilisation: beuip ls <pseudo>\n");
            return 1;
        }
        if (!serveur_actif) {
            printf("Connectez-vous au réseau (beuip start <pseudo>) avant de lister.\n");
            return 1;
        }
        demandeListe(argv[2]);

    } else if (strcmp(argv[1], "get") == 0) {
        if (argc < 4) {
            printf("Utilisation: beuip get <pseudo> <nomfic>\n");
            return 1;
        }
        if (!serveur_actif) {
            printf("Connectez-vous au réseau (beuip start <pseudo>) avant de télécharger.\n");
            return 1;
        }
        demandeFichier(argv[2], argv[3]);

    } else {
        printf("Commande beuip non reconnue.\n");
        return 1;
    }

    return 0;
}

void ajouteCom(char* Nom, int (*fonction)(int argc,char* argv[])) {
    if (NBCom >= NBMAXC) {
        perror("Max de commandes atteint");
        exit(-1);
    }
    Commande commande = {
        .Nom = Nom,
        .fonction = fonction
        };
    tableauCommande[NBCom] = commande;
    NBCom++;
}
void majComInt(void) {
    ajouteCom("exit",Sortie);
    ajouteCom("vers",Vers);
    ajouteCom("pwd",printWorkingDirectory);
    ajouteCom("cd",changeDirectory);
    ajouteCom("beuip",Beuip);
}
void listComInt(void) {
    for (int i = 0; i < NBCom; i++) {
        printf("%s\n",tableauCommande[i].Nom);
    }
}
int execComInt(int argc, char* argv[]) {
    if (argv[0]==NULL) {
        return 0;
    }
    for (int i = 0; i < NBCom; i++) {
        if (strcmp(tableauCommande[i].Nom,argv[0]) == 0 ) {
            tableauCommande[i].fonction(argc,argv);
            return 1;
        }
    }
    return 0;
}
int analyseCom(char* b) {
    if (Mots != NULL) {
        for (int i =0;i<NMots;i++) {
            free(Mots[i]);
        }
        free(Mots);
        Mots = NULL;
    }
    NMots = 0;
    const char* delimiteurs = " \t\n";
    char * chaine = strdup(b);
    char* reste = chaine;
    char* token;
    while ((token=strsep(&reste,delimiteurs))!=NULL) {
        if (*token!='\0') {
            NMots++;
        }
    }
    free(chaine);
    Mots = malloc((NMots+1)*sizeof(char*));
    int compteur = 0;
    chaine = strdup(b);
    reste = chaine;
    while ((token=strsep(&reste,delimiteurs))!=NULL) {
        if (*token!='\0') {
            Mots[compteur] = strdup(token);
            compteur++;
        }
    }
    Mots[compteur] = NULL;
    free(chaine);
    return NMots;
}
int execComExt(char** P) {
    if (P==NULL) {
        return 0;
    }
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(-1);
    }
    if (pid == 0) {
        execvp(P[0], P);
        perror(P[0]);
        exit(-1);
    }else {
        int status;
        waitpid(pid, &status, 0);
    }
    return 0;
}