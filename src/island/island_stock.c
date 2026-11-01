/***********************************************
 *
 * @File : island_stock.c
 * @Purpose : Shared stock of an island (buying, selling and saving to stock.db)
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/

#define _GNU_SOURCE
#include "island_stock.h"

/***********************************************
 *
 * @Name: saveStock
 * @Def: Rewrites the whole stock file with the current products.
 * @Arg: In: pstStock pointer to the stock (the caller must hold its mutex)
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
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

/***********************************************
 *
 * @Name: findProduct
 * @Def: Looks for a product by name, ignoring upper/lower case.
 * @Arg: In: pstStock pointer to the stock (the caller must hold its mutex)
 *       In: psName name of the product
 * @Ret: Returns a pointer to the product, or NULL if it is not found.
 *
 ***********************************************/
static tProduct *findProduct(tStock *pstStock, const char *psName) {
    for (int i = 0; i < pstStock->nNumProducts; i++) {
        if (strcasecmp(pstStock->pstProducts[i].sName, psName) == 0) {
            return &pstStock->pstProducts[i];
        }
    }
    return NULL;
}

/***********************************************
 *
 * @Name: stockInit
 * @Def: Initializes the stock with the products read from the file.
 * @Arg: Out: pstStock pointer to the stock to initialize
 *       In: pstProducts array of products (the stock takes its ownership)
 *       In: nNumProducts number of products in the array
 *       In: psPath path of the stock file, used to save the changes
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
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
    // From now on the stock owns the array (free in stockDestroy)
    pstStock->pstProducts = pstProducts;
    pstStock->nNumProducts = nNumProducts;
    return 0;
}

/***********************************************
 *
 * @Name: stockSnapshot
 * @Def: Makes a copy of all the products, to send them without holding the lock.
 * @Arg: In/Out: pstStock pointer to the stock
 *       Out: pnNumProducts pointer to store the number of products copied
 * @Ret: Returns a new array that the caller must free, or NULL on malloc failure.
 *
 ***********************************************/
tProduct *stockSnapshot(tStock *pstStock, int *pnNumProducts) {
    tProduct *pstCopy;

    // Copy while locked
    pthread_mutex_lock(&pstStock->stMutex);
    *pnNumProducts = pstStock->nNumProducts;
    pstCopy = malloc(sizeof(tProduct) * (pstStock->nNumProducts > 0 ? pstStock->nNumProducts : 1));
    if (pstCopy != NULL && pstStock->nNumProducts > 0) {
        memcpy(pstCopy, pstStock->pstProducts, sizeof(tProduct) * pstStock->nNumProducts);
    }
    pthread_mutex_unlock(&pstStock->stMutex);
    return pstCopy;
}

/***********************************************
 *
 * @Name: stockBuy
 * @Def: A ship buys kilograms of a product: they are removed from the stock
 *       and the change is saved to the file.
 * @Arg: In/Out: pstStock pointer to the stock
 *       In: psName name of the product
 *       In: nAmount kilograms to buy
 *       Out: pnCost pointer to store the total cost in gold
 *       Out: psCanonical buffer (MAX_NAME bytes) to store the product name as the island writes it
 * @Ret: Returns STOCK_OK on success, or the reason of the failure.
 *
 ***********************************************/
tStockResult stockBuy(tStock *pstStock, const char *psName, int nAmount, int *pnCost, char *psCanonical) {
    tStockResult eRes = STOCK_OK;
    tProduct *pstP;

    if (nAmount <= 0) {
        return STOCK_BAD_AMOUNT;
    }

    // Check, update and save while locked
    pthread_mutex_lock(&pstStock->stMutex);
    pstP = findProduct(pstStock, psName);
    if (pstP == NULL) {
        eRes = STOCK_NO_PRODUCT;
    } else if (pstP->nQuantity < nAmount) {
        eRes = STOCK_NOT_ENOUGH;
    } else {
        pstP->nQuantity -= nAmount;
        if (saveStock(pstStock) < 0) {
            // Undo: memory must match the file
            pstP->nQuantity += nAmount;
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

/***********************************************
 *
 * @Name: stockSell
 * @Def: A ship sells kilograms of a product: they are added to the stock
 *       and the change is saved to the file.
 * @Arg: In/Out: pstStock pointer to the stock
 *       In: psName name of the product
 *       In: nAmount kilograms to sell
 *       Out: pnIncome pointer to store the total income in gold
 *       Out: psCanonical buffer (MAX_NAME bytes) to store the product name as the island writes it
 * @Ret: Returns STOCK_OK on success, or the reason of the failure.
 *
 ***********************************************/
tStockResult stockSell(tStock *pstStock, const char *psName, int nAmount, int *pnIncome, char *psCanonical) {
    tStockResult eRes = STOCK_OK;
    tProduct *pstP;

    if (nAmount <= 0) {
        return STOCK_BAD_AMOUNT;
    }

    // Same locked step as in stockBuy
    pthread_mutex_lock(&pstStock->stMutex);
    pstP = findProduct(pstStock, psName);
    if (pstP == NULL) {
        // The island has no price for this product
        eRes = STOCK_NO_PRODUCT;
    } else {
        pstP->nQuantity += nAmount;
        if (saveStock(pstStock) < 0) {
            // Undo: memory must match the file
            pstP->nQuantity -= nAmount;
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

/***********************************************
 *
 * @Name: stockDestroy
 * @Def: Frees the products and the path of the stock and destroys its mutex.
 * @Arg: In/Out: pstStock pointer to the stock to destroy
 * @Ret: None.
 *
 ***********************************************/
void stockDestroy(tStock *pstStock) {
    free(pstStock->pstProducts);
    free(pstStock->psPath);
    pstStock->pstProducts = NULL;
    pstStock->psPath = NULL;
    pstStock->nNumProducts = 0;
    pthread_mutex_destroy(&pstStock->stMutex);
}