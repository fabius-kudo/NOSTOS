// island_stock.c
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include "island_stock.h"

// Rewrites the whole file. Caller must hold the mutex. Returns 0 or -1.
static int saveStock(const tStock *pstStock) {
    size_t nBytes = sizeof(tProduct) * (size_t)pstStock->nNumProducts;
    int nFd = open(pstStock->psPath, O_WRONLY | O_TRUNC);
    int nRet = 0;

    if (nFd < 0) {
        return -1;
    }
    if (nBytes > 0 && write(nFd, pstStock->pstProducts, nBytes) != (ssize_t)nBytes) {
        nRet = -1;
    }
    close(nFd);
    return nRet;
}

static tProduct *findProduct(tStock *pstStock, const char *psName) {
    for (int i = 0; i < pstStock->nNumProducts; i++) {
        if (strcasecmp(pstStock->pstProducts[i].sName, psName) == 0) {
            return &pstStock->pstProducts[i];
        }
    }
    return NULL;
}

int stockInit(tStock *pstStock, tProduct *pstProducts, int nNumProducts, const char *psPath) {
    pstStock->psPath = dupString(psPath);
    if (pstStock->psPath == NULL) {
        return -1;
    }
    if (pthread_mutex_init(&pstStock->stMutex, NULL) != 0) {
        free(pstStock->psPath);
        pstStock->psPath = NULL;
        return -1;
    }
    pstStock->pstProducts = pstProducts;        // takes ownership
    pstStock->nNumProducts = nNumProducts;
    return 0;
}

// Plain-value copy (tProduct has no pointers), so it can be sent without holding the lock
tProduct *stockSnapshot(tStock *pstStock, int *pnNumProducts) {
    tProduct *pstCopy;

    pthread_mutex_lock(&pstStock->stMutex);
    *pnNumProducts = pstStock->nNumProducts;
    pstCopy = malloc(sizeof(tProduct) * (pstStock->nNumProducts > 0 ? pstStock->nNumProducts : 1));
    if (pstCopy != NULL && pstStock->nNumProducts > 0) {
        memcpy(pstCopy, pstStock->pstProducts, sizeof(tProduct) * pstStock->nNumProducts);
    }
    pthread_mutex_unlock(&pstStock->stMutex);
    return pstCopy;                             // NULL on malloc failure
}

tStockResult stockBuy(tStock *pstStock, const char *psName, int nAmount, int *pnCost, char *psCanonical) {
    tStockResult eRes = STOCK_OK;
    tProduct *pstP;

    if (nAmount <= 0) {
        return STOCK_BAD_AMOUNT;
    }
    pthread_mutex_lock(&pstStock->stMutex);
    pstP = findProduct(pstStock, psName);
    if (pstP == NULL) {
        eRes = STOCK_NO_PRODUCT;
    } else if (pstP->nQuantity < nAmount) {
        eRes = STOCK_NOT_ENOUGH;
    } else {
        pstP->nQuantity -= nAmount;
        if (saveStock(pstStock) < 0) {
            pstP->nQuantity += nAmount;         // undo: memory must match the file
            eRes = STOCK_IO_ERROR;
        } else {
            *pnCost = nAmount * pstP->nPrice;
            strncpy(psCanonical, pstP->sName, MAX_NAME - 1);
            psCanonical[MAX_NAME - 1] = '\0';
        }
    }
    pthread_mutex_unlock(&pstStock->stMutex);
    return eRes;
}

tStockResult stockSell(tStock *pstStock, const char *psName, int nAmount, int *pnIncome, char *psCanonical) {
    tStockResult eRes = STOCK_OK;
    tProduct *pstP;

    if (nAmount <= 0) {
        return STOCK_BAD_AMOUNT;
    }
    pthread_mutex_lock(&pstStock->stMutex);
    pstP = findProduct(pstStock, psName);
    if (pstP == NULL) {
        eRes = STOCK_NO_PRODUCT;                    // the island has no price for it
    } else {
        pstP->nQuantity += nAmount;
        if (saveStock(pstStock) < 0) {
            pstP->nQuantity -= nAmount;             // undo: memory must match the file
            eRes = STOCK_IO_ERROR;
        } else {
            *pnIncome = nAmount * pstP->nPrice;
            strncpy(psCanonical, pstP->sName, MAX_NAME - 1);
            psCanonical[MAX_NAME - 1] = '\0';
        }
    }
    pthread_mutex_unlock(&pstStock->stMutex);
    return eRes;
}

void stockDestroy(tStock *pstStock) {
    free(pstStock->pstProducts);
    free(pstStock->psPath);
    pstStock->pstProducts = NULL;
    pstStock->psPath = NULL;
    pstStock->nNumProducts = 0;
    pthread_mutex_destroy(&pstStock->stMutex);
}