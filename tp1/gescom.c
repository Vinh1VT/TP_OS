#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "gescom.h"

static Commande tableauCommande[NBMAXC];
static int NBCom = 0;

char ** Mots = NULL;
int NMots = 0;

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
}

void listComInt(void) {
    for (int i = 0; i < NBCom; i++) {
        printf("%s",tableauCommande[i].Nom);
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
    char * chaine = strdup(b); // Remplacement demandé
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
    chaine = strdup(b); // Remplacement demandé
    reste = chaine;
    while ((token=strsep(&reste,delimiteurs))!=NULL) {
        if (*token!='\0') {
            Mots[compteur] = strdup(token); // Remplacement demandé
            compteur++;
        }

    }
    Mots[compteur] = NULL;
    free(chaine);


    //Renvoie le nombre de mots de la commande
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
        //Code du fils

        execvp(P[0], P);

        //SI ça ne termine pas
        perror(P[0]);
        exit(-1);
    }else {
        //Code du pere
        int status;

        waitpid(pid, &status, 0);

    }
    return 0;
}