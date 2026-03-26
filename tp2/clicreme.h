//
// Created by silen on 26/03/2026.
//

#ifndef BICEPS_CLICREME_H
#define BICEPS_CLICREME_H
#include <netdb.h>

int initSocket(struct sockaddr_in *Sock);
int clientBEUIP(char code, char* message, char *pseudo);
#endif //BICEPS_CLICREME_H