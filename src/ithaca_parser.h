/***********************************************
 *
 * @File : ithaca_parser.h
 * @Purpose : Header file for the Ithaca parser
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_ITHACA_PARSER_H
#define CODE_ITHACA_PARSER_H

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Project Includes
#include "custom_dynamic.h"

// Custom Type Definitions
typedef struct {
    char *psServerName;
    char *psPath;
    char *psIpAddress;
    int nPort;
} tIthaca;

typedef struct {
    char *psObject;
    char *psFile;
    char *psDestination;
    int nReward;
} tVoyage;

tIthaca parseIthaca(char *config_path);
void freeIthaca(tIthaca *ithaca);
tVoyage *parseVoyages(char *voyages_path, int *num_voyages);
void freeVoyages(tVoyage *voyages, int num_voyages);

#endif //CODE_ITHACA_PARSER_H