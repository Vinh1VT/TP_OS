
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include "clicreme.h"

#define PORT 9998
#define SERVEUR (127<<24|1)
#define LBUF 1024
#define DEBUG 0 // Mettre à 1 pour des messages de debug

/// Fonction de reception de l'ACK serveur
int ack(int sid, struct sockaddr_in *Sock) {
    char ack_buf[LBUF]; // Correction du buffer
    socklen_t ls = sizeof(*Sock);
    // Renvoie 1 si on a eu un ack, 0 sinon
    int n = recvfrom(sid, ack_buf, sizeof(ack_buf) - 1, 0, (struct sockaddr *)Sock, &ls);
    if (n > 0) {
        ack_buf[n] = '\0'; // On sécurise la chaîne de caractères
        return 1;
    } else {
        return 0;
    }
}

/// Envoie le message au serveur
int envoiMessage(int sid, struct sockaddr_in *Sock, char code, char *message, char* pseudo ) {
    char BufferMessage[LBUF];
    int LongueurMessage = 0;
    if (code == '3') {
        LongueurMessage = sprintf(BufferMessage,"3BEUIP");
    }else if (code == '4') {
        LongueurMessage = sprintf(BufferMessage,"4BEUIP%s",pseudo);
        strcpy(BufferMessage+LongueurMessage+ 1,message);
        LongueurMessage = LongueurMessage + 1 + strlen(message);
    }else if (code == '5') {
        LongueurMessage = sprintf(BufferMessage,"5BEUIP%s",message);
    }

    if (LongueurMessage) {
        if (sendto(sid, BufferMessage, LongueurMessage, 0, (struct sockaddr *)Sock, sizeof(*Sock)) == -1) {
            perror("sendto");
            return -1;
        }
    }
    return 0;
}

/// Initialise le socket
int initSocket(struct sockaddr_in *Sock) {
    int sid;
    /* creation du socket */
    if ((sid=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
        perror("socket");
        return -1;
    }
    bzero(Sock,sizeof(*Sock));
    Sock->sin_family = AF_INET;
    Sock->sin_addr.s_addr = htonl(SERVEUR);
    Sock->sin_port = htons(PORT);
    return sid;
}

/// Prend les commandes de l'interpréteur et les parse pour envoyer le message
int clientBEUIP(char code, char* message, char *pseudo) {
    // Initialisation du serveur
    struct sockaddr_in Sock;
    int sid = initSocket(&Sock);
    if (sid==-1) {
        return -1;
    }

    // Parsing et vérification de la commande
    int res = -1;
    if (code == '3') {
        res = envoiMessage(sid, &Sock, code, NULL,NULL);
    }else if (code == '4') {
         res = envoiMessage(sid, &Sock, code,message,pseudo);
    }else if (code == '5') {
         res = envoiMessage(sid, &Sock, code,message,NULL);
    }else {
        printf("Code non reconnu\n");
    }

    // ACK
    if (res == 0 ) {
        if (ack(sid, &Sock)) {
            if (DEBUG) printf("ACK RECU");
        }else {
            printf("Erreur d'ACK");
        }
    }

    close(sid);
    return res;
}




// /// Main pour l'utilisation en dehors de l'interpreteur
// int main(int N, char*P[])
// {
//     struct sockaddr_in Sock;
//     int sid = initSocket(&Sock);
//     if (sid==-1) {
//         return -1;
//     }
//
//     // Vérification des paramètres (N=3 pour code+message, N=4 pour code+pseudo+message)
//     if (N != 3 && N != 4) {
//         fprintf(stderr,"Utilisation 1 : %s code message\n", P[0]);
//         fprintf(stderr,"Utilisation 2 : %s code pseudo message\n", P[0]);
//         return(1);
//     }
//
//
//     /* ========================================================= */
//     /* Formatage du message BEUIP                                */
//     /* ========================================================= */
//     char payload[LBUF];
//     int payload_len = 0;
//
//     if (N == 3) {
//         // Commande : ./clibeuip code message
//         // Résultat attendu : codeBEUIPmessage
//         sprintf(payload, "%sBEUIP%s", P[1], P[2]);
//         payload_len = strlen(payload);
//     }
//     else if (N == 4) {
//         // Commande : ./clibeuip code pseudo message
//         // Résultat attendu : codeBEUIPpseudo\0message
//
//         // 1. On écrit d'abord la première partie (code + BEUIP + pseudo)
//         // sprintf renvoie le nombre de caractères écrits, ce qui nous donne l'offset (la position du \0 naturel)
//         int offset = sprintf(payload, "%sBEUIP%s", P[1], P[2]);
//
//         // 2. On calcule la taille totale : la première partie + 1 (pour garder le \0) + la longueur du message
//         payload_len = offset + 1 + strlen(P[3]);
//
//         // 3. On copie le message juste après le \0
//         strcpy(payload + offset + 1, P[3]);
//     }
//
//     /* Envoi du message formaté */
//     if (sendto(sid, payload, payload_len, 0, (struct sockaddr *)&Sock, sizeof(Sock)) == -1) {
//         perror("sendto");
//         return(4);
//     }
//     printf("Envoi OK !\n");
//
//     /* ========================================================= */
//     /* Réception de l'ACK                                        */
//     /* ========================================================= */
//     if (ack(sid, Sock) == 1) {
//         printf("ACK RECU \n");
//     }else{
//         printf("ACK NON RECU \n");
//     }
//
//     close(sid);
//     return 0;
// }