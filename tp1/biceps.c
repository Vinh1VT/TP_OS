#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <sys/wait.h>
#include <signal.h>
#include "gescom.h"
#include <readline/history.h>

char* generatePrompt() {
    char* user = getenv("USER");
    if (user==NULL) user = "user"; // Sécurité pour ne pas planter
    size_t len = 30;
    char* host = malloc(len*sizeof(char));

    if (gethostname(host,len)==-1) {
        perror("gethostname");
        free(host);
        return NULL ;
    }

    char root;
    if (getuid()==0) {
        root = '#';
    }else {
        root = '$';
    }


    char* prompt = malloc(sizeof(char)*(strlen(user)+strlen(host)+3));

    sprintf(prompt,"%s@%s%c",user,host,root);



    free(host);
    return prompt;
}

//Boucle principale de l'interpréteur
bool boucle() {
    char *prompt;
    char *command;
    prompt = generatePrompt();
    command = readline(prompt);
    free(prompt);

    if (command==NULL) {
        return false;
    }

    char *reste_cmd = command;
    char *sous_commande;

    while ((sous_commande = strsep(&reste_cmd, ";")) != NULL) {

        analyseCom(sous_commande);

        if (NMots > 0) {
            if (execComInt(NMots, Mots) == 0) {
                execComExt(Mots);
            }else {

            }
        }
    }

    free(command);

    return true;
}


int main(int argc, char** argv) {
    signal(SIGINT,SIG_IGN);
    majComInt();


    while (boucle()){};

    // Nettoyage final avant de quitter
    for (int i = 0; i < NMots; i++) {
        free(Mots[i]);
    }
    free(Mots);

    return 0;
}