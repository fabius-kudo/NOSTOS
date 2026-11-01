/***********************************************
 *
 * @File : island_stock.h
 * @Purpose : Header file for the island stock management
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/
#ifndef CODE_ISLAND_STOCK_H
#define CODE_ISLAND_STOCK_H

// System Includes
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>

// Project Includes
#include "port_config.h"            // tProduct

// Custom Type Definitions
typedef enum {
    STOCK_OK = 0,
    STOCK_BAD_AMOUNT,
    STOCK_NO_PRODUCT,
    STOCK_NOT_ENOUGH,
    STOCK_IO_ERROR,
    STOCK_NO_MEMORY
} tStockResult;

typedef struct {
    tProduct *pstProducts;          // owned: freed by stockDestroy
    int nNumProducts;
    char *psPath;                   // heap copy of the stock.db path
    pthread_mutex_t stMutex;        // protects the array and the file
} tStock;

// Prototypes
int stockInit(tStock *pstStock, tProduct *pstProducts, int nNumProducts, const char *psPath);
tProduct *stockSnapshot(tStock *pstStock, int *pnNumProducts);          // caller frees
tStockResult stockBuy(tStock *pstStock, const char *psName, int nAmount, int *pnCost, char *psCanonical);
tStockResult stockSell(tStock *pstStock, const char *psName, int nAmount, int *pnIncome, char *psCanonical);
void stockDestroy(tStock *pstStock);

#endif