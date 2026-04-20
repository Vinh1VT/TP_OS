#ifndef TP3_H
#define TP3_H

#include <stdbool.h>
#include <pthread.h>

#define LPSEUDO 23

struct elt {
    char nom[LPSEUDO+1];  /* nom de l'element */
    char adip[16];        /* IPv4 de l'element */
    struct elt * next;    /* adresse du prochain element */
};

// Variables globales pour la liste
extern struct elt * liste_contacts;
extern pthread_mutex_t mutex_contacts;
extern bool serveur_actif;

void ajouteElt(char * pseudo, char * adip);
void supprimeElt(char * adip);
void listeElts(void);
void * serveur_udp(void * p);
void commande(char octet1, char * message, char * pseudo);
void * serveur_tcp(void * rep);
void envoiContenu(int fd, char* rep);
void demandeListe(char * pseudo);
void demandeFichier(char * pseudo, char * nomfic);


#endif // TP3_H