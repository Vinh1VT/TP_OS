

#ifndef BICEPS_GESCOM_H
#define BICEPS_GESCOM_H

#define NBMAXC 10 /* Nb maxi de commandes internes */
#define VERSION 1.00

typedef struct s_Commande{
    char* Nom;
    int (*fonction)(int argc,char* argv[]);
}Commande;

/* Variables rendues globales car utilisées par le main() dans biceps.c */
extern char ** Mots;
extern int NMots;

/* Prototypes des commandes internes */
int Sortie(int argc,char* argv[]);
int Vers(int argc, char* argv[]);
int changeDirectory(int argc,char* argv[]);
int printWorkingDirectory(int argc,char* argv[]);

/* Prototypes des fonctions de gestion de commandes */
void ajouteCom(char* Nom, int (*fonction)(int argc,char* argv[]));
void majComInt(void);
void listComInt(void);
int execComInt(int argc, char* argv[]);
int analyseCom(char* b);
int execComExt(char** P);

#endif //BICEPS_GESCOM_H