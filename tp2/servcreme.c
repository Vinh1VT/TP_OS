/*****
* Exemple de serveur UDP
* socket en mode non connecte
*****/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "servcreme.h"

#define PORT 9998
#define LBUF 512
#define LOCALHOST ((127<<24) | 1)
#define IP_BASE ((192U << 24) | (168 << 16) | (88 << 8))
#define DEBUG 0 //Changer à 1 si on veut voir tout les messages reçus et les acks envoyés

struct s_Message {
    char code;
    char protocole[6];
    char pseudo[50];
};
typedef struct s_Message Message;

struct s_Couple {
    bool pris;
    char pseudo[50];
};

typedef struct s_Couple Couple;


char buf[LBUF+1];
struct sockaddr_in SockConf; /* pour les operation du serveur : mise a zero */

char * addrip(unsigned long A)
{
static char b[16];
  sprintf(b,"%u.%u.%u.%u",(unsigned int)(A>>24&0xFF),(unsigned int)(A>>16&0xFF),
         (unsigned int)(A>>8&0xFF),(unsigned int)(A&0xFF));
  return b;
}

/// Initialise le serveur
int initServer(struct sockaddr_in *socketConf) {
    int sid;
    if ((sid = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP))<0) {
        perror("socket");
        return -1;
    }
    socketConf->sin_family = AF_INET;
    socketConf->sin_port = htons(PORT);
    if (bind(sid, (struct sockaddr *) socketConf, sizeof(*socketConf)) == -1) {
        perror("bind");
        return -1;
    }
    printf("Le serveur est attache au port %d !\n",PORT);
    int broadcast = 1;
    if (setsockopt(sid,SOL_SOCKET,SO_BROADCAST,&broadcast,sizeof(broadcast))==-1) {
        perror("setsockopt");
        return -1;
    }

    return sid;

}

/// Envoie un message en broadcast sur le réseau
int sendBroadcastConnection(int sid,char Code,char* message) {

    struct sockaddr_in broadcastSock;
    broadcastSock.sin_family = AF_INET;
    broadcastSock.sin_port = htons(PORT);
    unsigned int ipBroadcast = IP_BASE | 255;
    broadcastSock.sin_addr.s_addr = htonl(ipBroadcast);

    sprintf(buf,"%c%s%s",Code,"BEUIP",message);

    if (sendto(sid,buf,strlen(buf),0,(struct sockaddr *) &broadcastSock,sizeof(broadcastSock))==-1) {
        perror("sendto");
        return -1;
    }

    return 0;
}

/// Envoie un accusé de réception (ACK) apres la réception d'un message
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

/// traitement des accusés de réceptions (entete 2)
void traitementAck(Couple couple[],struct sockaddr_in *Sock,char*pseudo) {
    int ip = ntohl(Sock->sin_addr.s_addr) & 0xFF;
    if (couple[ip].pris == false) {
        couple[ip].pris = true;
        strcpy(couple[ip].pseudo, pseudo);
        if (DEBUG) printf("Utilisateur découvert : %s (192.168.88.%d)\n", pseudo, ip);
    }
}

/// Afficher la liste des couples (entete 3)
void afficherListe(Couple couple[]) {
    for (int i = 0;i<255;i++) {
        if (couple[i].pris==true) {
            printf("192.168.88.%d : %s\n",i,couple[i].pseudo);
        }
    }
}

/// Envoie un message à un autre utilisateur (entete 4)
void envoyerMessage(int sid, Couple couple[],struct sockaddr_in *Sock, char* message) {
    if (ntohl(Sock->sin_addr.s_addr )!= LOCALHOST ) {
        //Sécurité, pour éviter l'usurpation d'identité
        printf("Impossible d'envoyer un message depuis une machine différente");
    }else {
        //On récupère le pseudo
        char* target_pseudo = message;
        int pseudo_len = strlen(target_pseudo);
        //On récupère le message
        char* messageText = target_pseudo +pseudo_len+1;
        bool found = false;
        struct sockaddr_in target_addr;
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons( PORT);

        for (int i = 0;i<255;i++) {
            if (couple[i].pris==true && strcmp(target_pseudo,couple[i].pseudo)==0) {
                unsigned int ip_dest = (192U << 24) | (168 << 16) | (88 << 8) | i;
                target_addr.sin_addr.s_addr = htonl(ip_dest);
                found = true;
                break;
            }
        }
        if (found) {
            char sendbuf[LBUF];
            int len = sprintf(sendbuf,"9BEUIP%s",messageText);

            if (sendto(sid,sendbuf,len,0,(struct sockaddr *) &target_addr,sizeof(target_addr))==-1) {
                perror("Sendto pseudo");
            }else {
                printf("Message transféré");
            }
        }else {
            printf("Utilisateur %s inconnu.\n", target_pseudo);
        }


    }
}

/// Reception d'un message (entete 9)
void recevoirMessage( Couple couple[],struct sockaddr_in *Sock, char* message) {
    int ip_index = ntohl(Sock->sin_addr.s_addr) & 0xFF;
    if (couple[ip_index].pris == true) {
        printf("\n[Message privé de %s] : %s\n", couple[ip_index].pseudo, message);
    } else {
        printf("\n[Message privé d'un inconnu (%s)] : %s\n",
               addrip(ntohl(Sock->sin_addr.s_addr)), message);
    }
}

/// Envoi d'un message à tout le réseau (entete 5)
void sendBroadcastMessage(int sid,struct sockaddr_in *Sock, char* message) {
    if (ntohl(Sock->sin_addr.s_addr )!= LOCALHOST) {
        //Sécurité, pour éviter l'usurpation d'identité
        printf("Impossible d'envoyer un message depuis une machine différente");
    }else {
        //On récupère le message
        char* messageText = message;
        struct sockaddr_in target_addr;
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons( PORT);
        unsigned int ip_dest = (192U << 24) | (168 << 16) | (88 << 8) | 255;
        target_addr.sin_addr.s_addr = htonl(ip_dest);

        char sendbuf[LBUF];
        int len = sprintf(sendbuf,"9BEUIP%s",messageText);

        if (sendto(sid,sendbuf,len,0,(struct sockaddr *) &target_addr,sizeof(target_addr))==-1) {
            perror("Sendto everyone");
        }else {
            printf("Message transféré à tout le monde");
        }
    }
}

/// Sortie du reseau (entete 0)
void quitterReseau(int sid, Couple couple[],struct sockaddr_in *Sock) {
    if (ntohl(Sock->sin_addr.s_addr )!= LOCALHOST) {
        //On enleve l'adresse reçue de la table de correspondance
        int ip_index = ntohl(Sock->sin_addr.s_addr) & 0xFF;

        if (couple[ip_index].pris == true) {
            couple[ip_index].pris = false;
        }


    }else {
        //On envoie un message broadcast pour sortir du programme
        struct sockaddr_in target_addr;
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons( PORT);
        unsigned int ip_dest = (192U << 24) | (168 << 16) | (88 << 8) | 255;
        target_addr.sin_addr.s_addr = htonl(ip_dest);

        char sendbuf[LBUF];
        int len = sprintf(sendbuf,"0BEUIPquit");

        if (sendto(sid,sendbuf,len,0,(struct sockaddr *) &target_addr,sizeof(target_addr))==-1) {
            perror("Sendto everyone");
        }else {
            printf("Message de sortie envoyé\n");
        }
    }
}

/// Machine à état du serveur
int gestionReception(int sid, int n, struct sockaddr_in *Sock, Couple couple[], char* pseudo) {
          buf[n] = '\0';
          if (DEBUG) printf ("recu de %s : <%s>\n",addrip(ntohl(Sock->sin_addr.s_addr)), buf);

          char entete = buf[0];
          char protocole[6];
          for (int i = 1;i<6;i++) {
              protocole[i-1] = buf[i];
          }
          protocole[5] = '\0';

          char message[LBUF];
          int taille_message = n - 6;

          if (taille_message > 0) {
              for (int i = 6; i < n; i++) {
                  message[i - 6] = buf[i];
              }
              message[taille_message] = '\0';
          } else {
              message[0] = '\0';
          }

          if (strcmp(protocole,"BEUIP")==0) {
              switch (entete) {
                  case '1': //Les cas 1 et 2 sont pareils, on traite les traite comme des ACKs
                  case '2':
                      traitementAck(couple,Sock,message);
                      break;
                  case '3':
                      afficherListe(couple);
                      break;
                  case '4':
                      envoyerMessage(sid,couple,Sock,message);
                      break;
                  case '9':
                      recevoirMessage(couple,Sock,message);
                      break;
                  case '5' :
                      sendBroadcastConnection(sid,'9',message);
                      break;
                  case '0':
                      quitterReseau(sid,couple,Sock);
                      return 1;
                  default: printf("entete non reconnu");
              }
              }
              if (entete!='2') sendAck(sid, Sock, pseudo);

        return 0;
}

/// Fonction principale, pour l'utilisation via l'interpréteur de commande
int serveurBEUIP(char*P)
{
int n;
struct sockaddr_in Sock;
socklen_t ls;
    Couple couple[255] = {0};


    int sid = initServer(&SockConf);

    if (sendBroadcastConnection(sid,'1',P)==-1) {
        return 4;
    }

    //Boucle principale
    for (;;) {
      ls = sizeof(Sock);
      /* on attend un message */
      if ((n=recvfrom(sid,(void*)buf,LBUF,0,(struct sockaddr *)&Sock,&ls))
           == -1)  perror("recvfrom");
      else {
        if (gestionReception(sid, n, &Sock, couple, P)) break;
      }
    }
    return 0;
}




/*
/// Main pour le serveur en dehors de l'interpréteur
int main(int N, char*P[])
{
int n;
struct sockaddr_in Sock;
socklen_t ls;
    Couple couple[255] = {0};

//Vérification que on à le pseudo en parametre
    if (N!=2) {
        perror("Utilisation : ./servbeuip <pseudo>");
        return 1;
    }

    int sid = initServer(&SockConf);

    if (sendBroadcastConnection(sid,'1',P[1])==-1) {
        return 4;
    }

    //Boucle principale
    for (;;) {
      ls = sizeof(Sock);
      if ((n=recvfrom(sid,(void*)buf,LBUF,0,(struct sockaddr *)&Sock,&ls))
           == -1)  perror("recvfrom");
      else {
        if (gestionReception(sid, n, &Sock, couple, P[1])) break;
      }
    }
    return 0;
}*/
