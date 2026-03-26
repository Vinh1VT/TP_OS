#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "gescom.h"
#include "../tp2/servcreme.h"
#include "../tp2/clicreme.h"

static Commande tableauCommande[NBMAXC];
static int NBCom = 0;
static int serverPID=-1;

char ** Mots = NULL;
int NMots = 0;

/// Construit un message à partir des argv séparés. Utile pour le client beuip
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

/// Démarrage et extinction du serveur BEUIP
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
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }
        if (pid == 0) {
            //Code du fils
            serveurBEUIP(argv[2]);
            exit(0);
        }else {
            //Code du père
            serverPID = pid;
            printf("Serveur beuip démarré avec le pseudo '%s' (PID: %d)\n", argv[2], pid);
        }
    }else if (strcmp(argv[1],"stop")==0) {
        if (serverPID == -1) {
            printf("Aucun serveur en cours d'exécution.\n");
            return 1;
        }
        struct sockaddr_in serv_addr;
        int sock = initSocket(&serv_addr);
        if (sock != -1) {
            sendto(sock, "0BEUIPquit", 10, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
            close(sock);
        }
        waitpid(serverPID, NULL, 0);
        serverPID = -1;
        printf("Serveur arrêté avec succès.\n");
        }
    return 0;
}

/// Envoi de messages par le client, au seveur BEUIP
int Mess(int argc,char* argv[]) {
    if (serverPID == -1 ) {
        printf("Connectez vous au réseau avant d'envoyer un message (beuip start <pseudo>)\n");
        return 1;
    }
    if (argc<2) {
        printf("utilisation : mess <commande>\n");
        return 1;
    }

    if (strcmp(argv[1],"list")==0) {
        return clientBEUIP('3',NULL,NULL);
    }else if (strcmp(argv[1],"send")==0) {
        if (argc < 4) {
            printf("utilisation : mess send <pseudo> <message>\n");
            return 1;
        }

        char message[1024];
        construireMessage(argc,argv,3,message);

        return clientBEUIP('4',message,argv[2]);
    }else if (strcmp(argv[1],"broadcast")==0) {
        if (argc < 3) {
            printf("utilisation : mess broadcast <message>\n");
            return 1;
        }
        char message[1024];
        construireMessage(argc,argv,2,message);
        return clientBEUIP('5',message,NULL);
    }
    printf("mess : commande non reconnue\n");
    return 1;
}

/// FONCTIONS



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
    ajouteCom("mess",Mess);
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

