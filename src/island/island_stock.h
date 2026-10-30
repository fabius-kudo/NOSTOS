#ifndef CODE_ISLAND_STOCK_H
#define CODE_ISLAND_STOCK_H

#include <pthread.h>
#include "port_config.h"            // tProduct

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

int stockInit(tStock *pstStock, tProduct *pstProducts, int nNumProducts, const char *psPath);
tProduct *stockSnapshot(tStock *pstStock, int *pnNumProducts);          // caller frees
tStockResult stockBuy(tStock *pstStock, const char *psName, int nAmount, int *pnCost, char *psCanonical);
tStockResult stockSell(tStock *pstStock, const char *psName, int nAmount, int *pnIncome, char *psCanonical);
void stockDestroy(tStock *pstStock);

#endif