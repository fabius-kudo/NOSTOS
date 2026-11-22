/***********************************************
 *
 * @File : ship_state.c
 * @Purpose : Saving and restoring the progress of an Odysseus between executions
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 22/11/26
 *
 ***********************************************/

#define _GNU_SOURCE

#include "ship_state.h"

// Written first, to recognise a valid state file
#define STATE_MARKER 0x4E535431

// Limits used to reject a damaged file instead of allocating nonsense
#define MAX_STATE_TEXT 4096
#define MAX_STATE_ITEMS 1000

/***********************************************
 *
 * @Name: statePath
 * @Def: Builds the path of the state file of a ship.
 * @Arg: In: pstShip ship state (name and folder from its config)
 * @Ret: Heap copy of the path (the caller frees it), or NULL on error.
 *
 ***********************************************/
char *statePath(const tShip *pstShip) {
    char *psPath = NULL;

    if (asprintf(&psPath, "%s/%s.state", pstShip->stConfig.psFilePath, pstShip->stConfig.psName) < 0) {
        return NULL;
    }
    return psPath;
}

/***********************************************
 *
 * @Name: writeInt
 * @Def: Writes a number to the state file. After the first failure it
 *       writes nothing more, so the caller only checks once at the end.
 * @Arg: In: nFd state file
 *       In: nValue number to write
 *       In/Out: pnOk 1 while every write has succeeded
 * @Ret: None.
 *
 ***********************************************/
void writeInt(int nFd, int nValue, int *pnOk) {
    if (*pnOk && write(nFd, &nValue, sizeof(int)) != (ssize_t)sizeof(int)) {
        *pnOk = 0;
    }
}

/***********************************************
 *
 * @Name: writeText
 * @Def: Writes a text as its length followed by its characters (length 0
 *       for NULL).
 * @Arg: In: nFd state file
 *       In: psText text to write, or NULL
 *       In/Out: pnOk 1 while every write has succeeded
 * @Ret: None.
 *
 ***********************************************/
void writeText(int nFd, const char *psText, int *pnOk) {
    int nLen = (psText == NULL) ? 0 : (int)strlen(psText);

    writeInt(nFd, nLen, pnOk);
    if (*pnOk && nLen > 0 && write(nFd, psText, nLen) != nLen) {
        *pnOk = 0;
    }
}

/***********************************************
 *
 * @Name: saveShipState
 * @Def: Writes the whole progress of the ship to its state file: gold,
 *       cargo, discovered islands and routes, active voyage and last port.
 * @Arg: In: pstShip ship state
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
int saveShipState(const tShip *pstShip) {
    const tActiveVoyage *pstV = &pstShip->stVoyage;
    char *psPath = statePath(pstShip);
    int nOk = 1;
    int nFd;

    if (psPath == NULL) {
        return -1;
    }
    nFd = open(psPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    free(psPath);
    if (nFd < 0) {
        return -1;
    }

    writeInt(nFd, STATE_MARKER, &nOk);
    writeInt(nFd, pstShip->nMoney, &nOk);
    writeInt(nFd, pstShip->stConfig.nNumFoodItems, &nOk);
    writeInt(nFd, pstShip->nNumIslands, &nOk);
    writeInt(nFd, pstShip->nNumRoutes, &nOk);
    writeInt(nFd, pstShip->nHasVoyage, &nOk);
    writeInt(nFd, pstV->nId, &nOk);
    writeInt(nFd, pstV->nReward, &nOk);
    writeInt(nFd, pstV->nDelivered, &nOk);

    writeText(nFd, pstShip->psLocation, &nOk);
    writeText(nFd, pstV->psObject, &nOk);
    writeText(nFd, pstV->psDestination, &nOk);
    writeText(nFd, pstV->psObjectPath, &nOk);
    writeText(nFd, pstV->psSignedPath, &nOk);

    for (int i = 0; i < pstShip->stConfig.nNumFoodItems; i++) {
        writeText(nFd, pstShip->stConfig.pstFoodSupplies[i].psProduct, &nOk);
        writeInt(nFd, pstShip->stConfig.pstFoodSupplies[i].nQuantity, &nOk);
    }
    for (int i = 0; i < pstShip->nNumIslands; i++) {
        writeText(nFd, pstShip->pstIslands[i].sName, &nOk);
        writeText(nFd, pstShip->pstIslands[i].sIp, &nOk);
        writeInt(nFd, pstShip->pstIslands[i].nPort, &nOk);
    }
    for (int i = 0; i < pstShip->nNumRoutes; i++) {
        writeText(nFd, pstShip->pstRoutes[i].sA, &nOk);
        writeText(nFd, pstShip->pstRoutes[i].sB, &nOk);
    }

    close(nFd);
    return nOk ? 0 : -1;
}

/***********************************************
 *
 * @Name: readInt
 * @Def: Reads a number from the state file.
 * @Arg: In: nFd state file
 *       Out: pnValue number read
 * @Ret: Returns 0 on success, -1 if the file ended or failed.
 *
 ***********************************************/
int readInt(int nFd, int *pnValue) {
    if (read(nFd, pnValue, sizeof(int)) != (ssize_t)sizeof(int)) {
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Name: readText
 * @Def: Reads a text written by writeText.
 * @Arg: In: nFd state file
 *       Out: ppsText heap copy of the text (the caller frees it), or NULL
 *            when it was saved as "none"
 * @Ret: Returns 0 on success, -1 if the file is damaged.
 *
 ***********************************************/
int readText(int nFd, char **ppsText) {
    int nLen;

    *ppsText = NULL;
    if (readInt(nFd, &nLen) != 0 || nLen < 0 || nLen > MAX_STATE_TEXT) {
        return -1;
    }
    if (nLen == 0) {
        return 0;
    }
    *ppsText = malloc(nLen + 1);
    if (*ppsText == NULL) {
        return -1;
    }
    if (read(nFd, *ppsText, nLen) != nLen) {
        free(*ppsText);
        *ppsText = NULL;
        return -1;
    }
    (*ppsText)[nLen] = '\0';
    return 0;
}

/***********************************************
 *
 * @Name: freeLoaded
 * @Def: Frees a state read from the file (all its texts and arrays). Used
 *       both when the file turns out to be damaged and to free the old
 *       state of the ship once the new one replaces it.
 * @Arg: In/Out: pstShip ship-shaped holder of the state to free
 * @Ret: None.
 *
 ***********************************************/
void freeLoaded(tShip *pstShip) {
    free(pstShip->psLocation);
    free(pstShip->stVoyage.psObject);
    free(pstShip->stVoyage.psDestination);
    free(pstShip->stVoyage.psObjectPath);
    free(pstShip->stVoyage.psSignedPath);
    for (int i = 0; i < pstShip->stConfig.nNumFoodItems; i++) {
        free(pstShip->stConfig.pstFoodSupplies[i].psProduct);
    }
    free(pstShip->stConfig.pstFoodSupplies);
    for (int i = 0; i < pstShip->nNumIslands; i++) {
        free(pstShip->pstIslands[i].sName);
        free(pstShip->pstIslands[i].sIp);
    }
    free(pstShip->pstIslands);
    for (int i = 0; i < pstShip->nNumRoutes; i++) {
        free(pstShip->pstRoutes[i].sA);
        free(pstShip->pstRoutes[i].sB);
    }
    free(pstShip->pstRoutes);
}

/***********************************************
 *
 * @Name: readState
 * @Def: Reads the whole state file into a separate holder, so the ship is
 *       only changed once everything has been read correctly.
 * @Arg: In: nFd state file
 *       Out: pstNew holder for the state read (only the saved fields are used)
 * @Ret: Returns 0 on success, -1 if the file is damaged (pstNew is then
 *       left empty, with everything it had read already freed).
 *
 ***********************************************/
int readState(int nFd, tShip *pstNew) {
    int nMarker = 0;
    int nNumFood = 0;
    int nNumIslands = 0;
    int nNumRoutes = 0;
    int nOk;

    memset(pstNew, 0, sizeof(*pstNew));
    nOk = (readInt(nFd, &nMarker) == 0 && nMarker == STATE_MARKER &&
           readInt(nFd, &pstNew->nMoney) == 0 &&
           readInt(nFd, &nNumFood) == 0 && readInt(nFd, &nNumIslands) == 0 &&
           readInt(nFd, &nNumRoutes) == 0 &&
           readInt(nFd, &pstNew->nHasVoyage) == 0 &&
           readInt(nFd, &pstNew->stVoyage.nId) == 0 &&
           readInt(nFd, &pstNew->stVoyage.nReward) == 0 &&
           readInt(nFd, &pstNew->stVoyage.nDelivered) == 0);
    if (!nOk || nNumFood < 0 || nNumFood > MAX_STATE_ITEMS || nNumIslands < 1 ||
        nNumIslands > MAX_STATE_ITEMS || nNumRoutes < 0 || nNumRoutes > MAX_STATE_ITEMS) {
        return -1;
    }

    nOk = (readText(nFd, &pstNew->psLocation) == 0 &&
           readText(nFd, &pstNew->stVoyage.psObject) == 0 &&
           readText(nFd, &pstNew->stVoyage.psDestination) == 0 &&
           readText(nFd, &pstNew->stVoyage.psObjectPath) == 0 &&
           readText(nFd, &pstNew->stVoyage.psSignedPath) == 0);

    // The arrays are allocated whole and counted up as they are filled, so
    // freeLoaded only frees the entries that were really read
    if (nOk && nNumFood > 0) {
        pstNew->stConfig.pstFoodSupplies = malloc(sizeof(tFood) * nNumFood);
        nOk = (pstNew->stConfig.pstFoodSupplies != NULL);
    }
    for (int i = 0; nOk && i < nNumFood; i++) {
        tFood *pstFood = &pstNew->stConfig.pstFoodSupplies[i];
        nOk = (readText(nFd, &pstFood->psProduct) == 0 && pstFood->psProduct != NULL);
        if (nOk) {
            pstNew->stConfig.nNumFoodItems++;
            nOk = (readInt(nFd, &pstFood->nQuantity) == 0);
        } else {
            free(pstFood->psProduct);
        }
    }

    if (nOk) {
        pstNew->pstIslands = malloc(sizeof(tKnownIsland) * nNumIslands);
        nOk = (pstNew->pstIslands != NULL);
    }
    for (int i = 0; nOk && i < nNumIslands; i++) {
        tKnownIsland *pstIsland = &pstNew->pstIslands[i];
        pstIsland->sIp = NULL;
        nOk = (readText(nFd, &pstIsland->sName) == 0 && readText(nFd, &pstIsland->sIp) == 0 &&
               pstIsland->sName != NULL && pstIsland->sIp != NULL);
        if (nOk) {
            pstNew->nNumIslands++;
            nOk = (readInt(nFd, &pstIsland->nPort) == 0);
        } else {
            free(pstIsland->sName);
            free(pstIsland->sIp);
        }
    }

    if (nOk && nNumRoutes > 0) {
        pstNew->pstRoutes = malloc(sizeof(tKnownRoute) * nNumRoutes);
        nOk = (pstNew->pstRoutes != NULL);
    }
    for (int i = 0; nOk && i < nNumRoutes; i++) {
        tKnownRoute *pstRoute = &pstNew->pstRoutes[i];
        pstRoute->sB = NULL;
        nOk = (readText(nFd, &pstRoute->sA) == 0 && readText(nFd, &pstRoute->sB) == 0 &&
               pstRoute->sA != NULL && pstRoute->sB != NULL);
        if (nOk) {
            pstNew->nNumRoutes++;
        } else {
            free(pstRoute->sA);
            free(pstRoute->sB);
        }
    }

    // A voyage needs its texts
    if (nOk && pstNew->nHasVoyage &&
        (pstNew->stVoyage.psObject == NULL || pstNew->stVoyage.psDestination == NULL)) {
        nOk = 0;
    }

    if (!nOk) {
        freeLoaded(pstNew);
        memset(pstNew, 0, sizeof(*pstNew));
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Name: fileExists
 * @Def: Checks that a file can be opened for reading.
 * @Arg: In: psPath path of the file, or NULL
 * @Ret: Returns 1 if it exists, 0 otherwise.
 *
 ***********************************************/
int fileExists(const char *psPath) {
    int nFd;

    if (psPath == NULL) {
        return 0;
    }
    nFd = open(psPath, O_RDONLY);
    if (nFd < 0) {
        return 0;
    }
    close(nFd);
    return 1;
}

/***********************************************
 *
 * @Name: loadShipState
 * @Def: Restores the progress saved by a previous execution, if any. The
 *       ship is only changed if the whole file is valid. The restored ship
 *       is not connected anywhere. A voyage whose object (or, once
 *       delivered, its stamped image) is no longer in the folder is dropped.
 * @Arg: In/Out: pstShip ship already initialised from its config
 * @Ret: STATE_NONE, STATE_RESTORED, STATE_VOYAGE_LOST or STATE_DAMAGED.
 *
 ***********************************************/
int loadShipState(tShip *pstShip) {
    tShip stNew;
    tShip stOld;
    char *psPath = statePath(pstShip);
    int nFd;
    int nRes;
    int nVoyageLost = 0;

    if (psPath == NULL) {
        return STATE_DAMAGED;
    }
    nFd = open(psPath, O_RDONLY);
    free(psPath);
    if (nFd < 0) {
        return STATE_NONE;
    }
    nRes = readState(nFd, &stNew);
    close(nFd);
    if (nRes != 0) {
        return STATE_DAMAGED;
    }

    // The voyage only continues if its image is still on board
    if (stNew.nHasVoyage) {
        if (stNew.stVoyage.nDelivered) {
            nVoyageLost = !fileExists(stNew.stVoyage.psSignedPath);
        } else {
            nVoyageLost = !fileExists(stNew.stVoyage.psObjectPath);
        }
    }
    if (nVoyageLost) {
        free(stNew.stVoyage.psObject);
        free(stNew.stVoyage.psDestination);
        free(stNew.stVoyage.psObjectPath);
        free(stNew.stVoyage.psSignedPath);
        memset(&stNew.stVoyage, 0, sizeof(stNew.stVoyage));
        stNew.nHasVoyage = 0;
    }

    // Swap: the old progress (from the config) is freed, the saved one is kept
    memset(&stOld, 0, sizeof(stOld));
    stOld.psLocation = pstShip->psLocation;
    stOld.stVoyage = pstShip->stVoyage;
    stOld.stConfig.pstFoodSupplies = pstShip->stConfig.pstFoodSupplies;
    stOld.stConfig.nNumFoodItems = pstShip->stConfig.nNumFoodItems;
    stOld.pstIslands = pstShip->pstIslands;
    stOld.nNumIslands = pstShip->nNumIslands;
    stOld.pstRoutes = pstShip->pstRoutes;
    stOld.nNumRoutes = pstShip->nNumRoutes;
    freeLoaded(&stOld);

    pstShip->nMoney = stNew.nMoney;
    pstShip->psLocation = stNew.psLocation;
    pstShip->nHasVoyage = stNew.nHasVoyage;
    pstShip->stVoyage = stNew.stVoyage;
    pstShip->stConfig.pstFoodSupplies = stNew.stConfig.pstFoodSupplies;
    pstShip->stConfig.nNumFoodItems = stNew.stConfig.nNumFoodItems;
    pstShip->pstIslands = stNew.pstIslands;
    pstShip->nNumIslands = stNew.nNumIslands;
    pstShip->pstRoutes = stNew.pstRoutes;
    pstShip->nNumRoutes = stNew.nNumRoutes;

    return nVoyageLost ? STATE_VOYAGE_LOST : STATE_RESTORED;
}

/***********************************************
 *
 * @Name: deleteShipState
 * @Def: Erases the progress of a ship lost at sea: its state file and the
 *       images of its voyage. The next execution starts from the config.
 * @Arg: In: pstShip ship state
 * @Ret: None.
 *
 ***********************************************/
void deleteShipState(const tShip *pstShip) {
    char *psPath = statePath(pstShip);

    if (psPath != NULL) {
        unlink(psPath);
        free(psPath);
    }
    if (pstShip->stVoyage.psObjectPath != NULL) {
        unlink(pstShip->stVoyage.psObjectPath);
    }
    if (pstShip->stVoyage.psSignedPath != NULL) {
        unlink(pstShip->stVoyage.psSignedPath);
    }
}