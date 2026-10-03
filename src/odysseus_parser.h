/***********************************************
 *
 * @File : odysseus_parser.h
 * @Purpose : Header file for the parser for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_ODYSSEUS_PARSER_H
#define CODE_ODYSSEUS_PARSER_H

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
    char *psProduct;
    int nQuantity;
} tFood;

typedef struct {
    char *psName;
    char *psFilePath;

    char *psIthacaIp;
    int nIthacaPort;

    char *psIslandName;
    char *psIslandIp;
    int nIslandPort;

    int nInitialMoney;
    int nNumFoodItems;
    tFood *pstFoodSupplies;
} tOdysseus;

tOdysseus parseOdysseus(char *psConfigPath);
void freeOdysseus(tOdysseus *pstOd);

#endif //CODE_ODYSSEUS_PARSER_H