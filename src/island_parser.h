/***********************************************
 *
 * @File : island_parser.h
 * @Purpose : Header file for the island parser
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_ISLAND_PARSER_H
#define CODE_ISLAND_PARSER_H

 // System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Project Includes
#include "custom_dynamic.h"
#include "SPHRAGIS Library-20260917/sphragis.h"

// Constant Declarations
#define MAX_NAME 100

// Custom Type Definitions
typedef struct {
    char sName[MAX_NAME];
    int nQuantity;
    int nPrice;
} tProduct;

typedef struct {
    char *psName;
    char *psIpAddress;
    int nPort;
} tRoute;

typedef struct {
    char *psName;
    char *psPath;
    char *psIpAddress;
    int nPort;
    int nMaxCapacity;
    tRoute *pstRoutes;
    int nNumRoutes;
} tIsland;

tIsland parseIsland(char *psIslandsPath);
void freeIsland(tIsland stIsland);
tProduct *parseStock(char *psStockPath, int *pnNumProducts);
int filterRoutes(tIsland *pstIsland);

#endif //CODE_ISLAND_PARSER_H