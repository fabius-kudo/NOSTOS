/***********************************************
 *
 * @File : ship_config.h
 * @Purpose : Header file for the parser for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_SHIP_CONFIG_H
#define CODE_SHIP_CONFIG_H

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

typedef struct {
    int nId;
    char *psOrigin;
    char *psDestination;
    int nReward;
} tActiveVoyage;

typedef struct {
    char *sName;
    char *sIp;
    int nPort;
} tKnownIsland;

typedef struct {
    char *sA;
    char *sB;
} tKnownRoute;

typedef struct {
    tOdysseus stConfig;
    int nMoney;
    int nIthacaFd;
    int nIslandFd;
    char *psLocation;
    int nHasVoyage;
    tActiveVoyage stVoyage;

    tKnownIsland *pstIslands;   
    int nNumIslands;   // starts with Aeaea from the config
    tKnownRoute *pstRoutes;     
    int nNumRoutes;    // starts empty
} tShip;

tOdysseus parseOdysseus(char *psConfigPath);
int initShip(tShip *pstShip, tOdysseus stConfig);
void freeOdysseus(tOdysseus *pstOd);

#endif