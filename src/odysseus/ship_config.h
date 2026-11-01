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
#include "custom_io.h"

// Custom Type Definitions

//One product carried in the cargo
typedef struct {
    char *psProduct;
    int nQuantity;
} tFood;

//Contents of odysseus.dat
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

// Voyage accepted at Ithaca
typedef struct {
    int nId;
    char *psObject;
    char *psDestination;
    int nReward;
} tActiveVoyage;

// An island the ship knows how to reach
typedef struct {
    char *sName;
    char *sIp;
    int nPort;
} tKnownIsland;

// A known direct route between two islands (bidirectional)
typedef struct {
    char *sA;
    char *sB;
} tKnownRoute;

//Complete state of the ship while the program runs
typedef struct {
    tOdysseus stConfig;
    int nMoney;
    int nIthacaFd;
    int nIslandFd;
    char *psLocation;
    int nHasVoyage;
    tActiveVoyage stVoyage;

    tKnownIsland *pstIslands;   
    int nNumIslands;
    tKnownRoute *pstRoutes;     
    int nNumRoutes;
    int nShouldExit;
} tShip;

tOdysseus parseOdysseus(char *psConfigPath);
int initShip(tShip *pstShip, tOdysseus stConfig);
void freeOdysseus(tOdysseus *pstOd);

#endif