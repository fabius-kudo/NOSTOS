#ifndef CODE_ITHICA_H
#define CODE_ITHICA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_NAME 50
#define MAX_PATH 256
#define IP_LENGTH 16

typedef struct {
    char serverName[MAX_NAME];
    char path[MAX_PATH];
    char ipAddress[IP_LENGTH];
    int port;
} Ithaca;

typedef struct {
    char object[MAX_NAME];
    char file[MAX_PATH];
    char destination[MAX_NAME];
    int reward;
} Voyage;

#endif //CODE_ITHICA_H
