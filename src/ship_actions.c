#include "ship_actions.h"
#include "ship_config.h"
#include "protocol.h"
#include "socket_utils.h"

int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    tFrame stReply;

    if (pstShip->nIthacaFd != -1) {
        write(STDOUT_FILENO, "Already connected to Ithaca\n", strlen("Already connected to Ithaca\n"));
        return 0;
    }

    int nFd = connectToServer(pstShip->stConfig.psIthacaIp, pstShip->stConfig.nIthacaPort);
    if (nFd < 0) {
        write(STDOUT_FILENO, "Error: cannot reach Ithaca.\n", strlen("Error: cannot reach Ithaca.\n"));
        return -1;
    }

    // 0x01 request carrying the Odysseus name
    sendFrame(nFd, TYPE_CONNECT_ITHACA, FLAG_REQUEST, pstShip->stConfig.psName, strlen(pstShip->stConfig.psName));

    if (recvFrame(nFd, &stReply) != 1 || stReply.ucFlags != FLAGS_OK) {
        write(STDOUT_FILENO, "Error: Ithaca refused the connection.\n", strlen("Error: Ithaca refused the connection.\n"));
        close(nFd);
        return -1;
    }

    pstShip->nIthacaFd = nFd;
    strcpy(pstShip->psLocation, "Ithaca");
    write(STDOUT_FILENO, "Connected to Ithaca.\n", strlen("Connected to Ithaca.\n"));
    return 0;
}

int cmdListVoyages(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void) nArgc;
    (void) ppsArgv;

    tFrame stFrame;
    char sData[FRAME_DATA_SIZE + 1];
    char sLine[300]; //TODO: make this dynamic
    int nLen;

    if (pstShip->nIthacaFd < 0) {
        write(STDOUT_FILENO, "Error: not connected to Ithaca.\n", strlen("Error: not connected to Ithaca.\n"));
        return -1;
    }

    sendFrame(pstShip->nIthacaFd, TYPE_LIST_VOYAGES, FLAG_REQUEST, NULL, 0);

    int nIndex = 0;
    int nTotal = 1;
    while (nIndex < nTotal) {
        if (recvFrame(pstShip->nIthacaFd, &stFrame) != 1) {
            write(STDOUT_FILENO, "Error: connection to Ithaca lost.\n", strlen("Error: connection to Ithaca lost.\n"));
            closeSocket(&pstShip->nIthacaFd);
            return -1;
        }
        frameDataToString(&stFrame, sData, sizeof(sData));

        if (stFrame.ucFlags != FLAGS_OK) {              // error: data is the reason
            nLen = snprintf(sLine, sizeof(sLine), "Error: %s\n", sData);
            write(STDOUT_FILENO, sLine, nLen);
            return -1;
        }

        char *psIndex = strtok(sData, "&");
        char *psTotal = strtok(NULL, "&");

        if (psTotal == NULL) {
            write(STDOUT_FILENO, "No voyages available.\n", strlen("No voyages available.\n"));
            return 0;
        }
        char *psId = strtok(NULL, "&");
        char *psObject = strtok(NULL, "&");
        char *psDest = strtok(NULL, "&");
        char *psReward = strtok(NULL, "&");

        if (!psId || !psObject || !psDest || !psReward) {
            write(STDOUT_FILENO, "Error: malformed reply from Ithaca.\n", strlen("Error: malformed reply from Ithaca.\n"));
            return -1;
        }

        nIndex = atoi(psIndex);
        nTotal = atoi(psTotal);
        if (nIndex == 1) {
            write(STDOUT_FILENO, "--- AVAILABLE VOYAGES ---\n", strlen("--- AVAILABLE VOYAGES ---\n"));
        }
        nLen = snprintf(sLine, sizeof(sLine), "%s. %s -> %s [%s]\n",
                        psId, psObject, psDest, psReward);
        write(STDOUT_FILENO, sLine, nLen);
    }
    return 0;
}

int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void) nArgc;

    int nLen;
    char *sMsg;
    tFrame stReply;
    char sData[FRAME_DATA_SIZE + 1];
    char *psOk, *psId, *psOrigin, *psDest, *psReward;

    if (pstShip->nIthacaFd < 0) {
        write(STDOUT_FILENO, "Error: not connected to Ithaca.\n", strlen("Error: not connected to Ithaca.\n"));
        return -1;
    }
    if (pstShip->nHasVoyage) {
        write(STDOUT_FILENO, "Error: you already have an active voyage.\n", strlen("Error: you already have an active voyage.\n"));
        return -1;
    }

    sendFrame(pstShip->nIthacaFd, TYPE_ACCEPT, FLAG_REQUEST, ppsArgv[0], (int)strlen(ppsArgv[0]));

    //Get response from Ithaca
    if (recvFrame(pstShip->nIthacaFd, &stReply) != 1) {
        write(STDOUT_FILENO, "Error: connection to Ithaca lost.\n", strlen("Error: connection to Ithaca lost.\n"));
        return -1;
    }

    frameDataToString(&stReply, sData, sizeof(sData));

    if (stReply.ucFlags != FLAGS_OK) {
        nLen = snprintf(sMsg, sizeof(sMsg), "Error: %s\n", sData);   // the reason
        write(STDOUT_FILENO, sMsg, nLen);
        return -1;
    }

    psOk = strtok(sData, "&");
    psId = strtok(NULL, "&");
    psOrigin = strtok(NULL, "&");
    psDest = strtok(NULL, "&");
    psReward = strtok(NULL, "&");
    if (!psOk || !psId || !psOrigin || !psDest || !psReward) {
        write(STDOUT_FILENO, "Error: malformed reply from Ithaca.\n", strlen("Error: malformed reply from Ithaca.\n"));
        return -1;
    }

    pstShip->stVoyage.nId = atoi(psId);
    strncpy(pstShip->stVoyage.sOrigin, psOrigin, sizeof(pstShip->stVoyage.sOrigin) - 1);
    pstShip->stVoyage.sOrigin[sizeof(pstShip->stVoyage.sOrigin) - 1] = '\0';
    strncpy(pstShip->stVoyage.sDestination, psDest, sizeof(pstShip->stVoyage.sDestination) - 1);
    pstShip->stVoyage.sDestination[sizeof(pstShip->stVoyage.sDestination) - 1] = '\0';
    pstShip->stVoyage.nReward = atoi(psReward);
    pstShip->nHasVoyage = 1;

    nLen = snprintf(sMsg, sizeof(sMsg), "Voyage %d accepted.\nDestination: %s.\n", pstShip->stVoyage.nId, pstShip->stVoyage.sDestination);
    write(STDOUT_FILENO, sMsg, nLen);

    return 0;
}

static const tKnownIsland *findKnownIsland(const tShip *pstShip, const char *psName) {
    for (int i = 0; i < pstShip->nNumIslands; i++) {
        if (strcmp(pstShip->pstIslands[i].sName, psName) == 0) {
            return &pstShip->pstIslands[i];
        }
    }
    return NULL;
}

static int isDirectRoute(const tShip *pstShip, const char *psFrom, const char *psTo) {
    for (int i = 0; i < pstShip->nNumRoutes; i++) {
        const tKnownRoute *pstR = &pstShip->pstRoutes[i];
        if ((strcmp(pstR->sA, psFrom) == 0 && strcmp(pstR->sB, psTo) == 0) ||
            (strcmp(pstR->sA, psTo) == 0 && strcmp(pstR->sB, psFrom) == 0)) {
            return 1;
        }
    }
    return 0;
}

static void leaveCurrentPort(tShip *pstShip) {
    tFrame stReply;
    int *pnFd;
    unsigned char ucType;

    if (pstShip->nIthacaFd >= 0) {
        pnFd = &pstShip->nIthacaFd;
        ucType = TYPE_DISCONNECT;
    } else if (pstShip->nIslandFd >= 0) {
        pnFd = &pstShip->nIslandFd;
        ucType = TYPE_ISLAND_LEAVE;
    } else {
        // already at sea
        return;
    }

    if (sendFrame(*pnFd, ucType, FLAG_REQUEST, NULL, 0) == 0) {
        recvFrame(*pnFd, &stReply);             // wait for the confirmation; we leave either way
    }
    closeSocket(pnFd);
}

static int totalProvisions(const tShip *pstShip) {
    int nTotal = 0;
    for (int i = 0; i < pstShip->stConfig.nNumFoodItems; i++) {
        nTotal += pstShip->stConfig.pstFoodSupplies[i].nQuantity;
    }
    return nTotal;
}

//TODO: make less ai looking
static void consumeProvisions(tShip *pstShip, int nKg) {
    for (int i = 0; i < pstShip->stConfig.nNumFoodItems && nKg > 0; i++) {
        tFood *pstFood = &pstShip->stConfig.pstFoodSupplies[i];
        int nTake = (pstFood->nQuantity < nKg) ? pstFood->nQuantity : nKg;
        pstFood->nQuantity -= nTake;
        nKg -= nTake;
    }
}

static int waitForDocking(tShip *pstShip, const char *psIsland) {
    tFrame stFrame;
    char sData[FRAME_DATA_SIZE + 1];
    char *sMsg;
    int nLen;
    
    //TODO: make this 5 second waiting work
    while (1) {
        sleep(5);

        if (recvFrame(pstShip->nIslandFd, &stFrame) != 1) {
            write(STDOUT_FILENO, "Error: connection to the island lost.\n", 38);
            closeSocket(&pstShip->nIslandFd);
            return 0;
        }
        frameDataToString(&stFrame, sData, sizeof(sData));
        if (stFrame.ucType == TYPE_ISLAND_ARRIVE && stFrame.ucFlags == FLAGS_OK && strcmp(sData, "DOCKED") == 0) {
            nLen = snprintf(sMsg, sizeof(sMsg), "\n%s has granted access to the port.\n", psIsland);
            write(STDOUT_FILENO, sMsg, nLen);
            return 1;
        }

        consumeProvisions(pstShip, RATION_KG);

        nLen = snprintf(sMsg, sizeof(sMsg), "\n>>> The crew consumes another ration.\nProvisions remaining: %d kg\n", totalProvisions(pstShip));
        write(STDOUT_FILENO, sMsg, nLen);

        if (totalProvisions(pstShip) <= 0) {
            nLen = snprintf(sMsg, sizeof(sMsg),"\nThe crew has run out of provisions.\nOdysseus %s has been lost at sea.\n", pstShip->stConfig.psName);
            write(STDOUT_FILENO, sMsg, nLen);
            closeSocket(&pstShip->nIslandFd);
            return 0;
        }
    }
}

int cmdSail(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void)nArgc;

    const tKnownIsland *pstDest;
    tFrame stReply;
    char sData[FRAME_DATA_SIZE + 1];
    char *sMsg;
    char *sFrom;
    int nLen, nFd, nFromIthaca;
    if (pstShip->psLocation[0] == '\0') {
        write(STDOUT_FILENO, "Error: you are not at any port.\n", strlen("Error: you are not at any port.\n"));
        return -1;
    }

    //Get the destination from the command arguments
    nFromIthaca = (strcasecmp(pstShip->psLocation, "Ithaca") == 0);
    pstDest = findKnownIsland(pstShip, ppsArgv[0]);

    //Check if the destination is valid and reachable from the current location
    if (pstDest == NULL || (nFromIthaca ? strcasecmp(pstDest->sName, pstShip->stConfig.psIslandName) != 0 : !isDirectRoute(pstShip, pstShip->pstIslands, pstDest->sName))) {
        nLen = snprintf(sMsg, sizeof(sMsg), "We can't sail to %s from here...\n", ppsArgv[0]);
        write(STDOUT_FILENO, sMsg, nLen);
        return -1;
    }

    //Diconnect from Ithaca/current island
    strncpy(sFrom, pstShip->psLocation, sizeof(sFrom) - 1);
    sFrom[sizeof(sFrom) - 1] = '\0';
    nLen = snprintf(sMsg, sizeof(sMsg), "Leaving %s...\nSailing to %s...\n", sFrom, pstDest->sName);
    write(STDOUT_FILENO, sMsg, nLen);
    leaveCurrentPort(pstShip);

    //Arrive at the destination island
    nFd = connectToServer(pstDest->sIp, pstDest->nPort);
    if (nFd < 0) {
        nLen = snprintf(sMsg, sizeof(sMsg), "Error: cannot reach %s.\n", pstDest->sName);
        write(STDOUT_FILENO, sMsg, nLen);
        return -1;                              // at sea; sLocation still holds the last port
    }

    sendTextFrame(nFd, TYPE_ISLAND_ARRIVE, FLAG_REQUEST, pstShip->stConfig.psName);

    if (recvFrame(nFd, &stReply) != 1) {
        write(STDOUT_FILENO, "Error: the island did not answer.\n", strlen("Error: the island did not answer.\n"));
        close(nFd);
        return -1;
    }
    frameDataToString(&stReply, sData, sizeof(sData));
    if (stReply.ucFlags != FLAGS_OK) {          // data is the reason
        nLen = snprintf(sMsg, sizeof(sMsg), "Error: %s\n", sData);
        write(STDOUT_FILENO, sMsg, nLen);
        close(nFd);
        return -1;
    }

    pstShip->nIslandFd = nFd;
    nLen = snprintf(sMsg, sizeof(sMsg), "Connected to %s.\n", pstDest->sName);
    write(STDOUT_FILENO, sMsg, nLen);

    //Check if the island has space to dock the ship
    if (strcmp(sData, "WAIT") == 0) {
        write(STDOUT_FILENO, "The harbor is full. We drop anchor and wait.\n", strlen("The harbor is full. We drop anchor and wait.\n"));
        if (!waitForDocking(pstShip, pstDest->sName)) {
            return -1;
        }
    } else {
        nLen = snprintf(sMsg, sizeof(sMsg), "%s has granted access to the port.\n", pstDest->sName);
        write(STDOUT_FILENO, sMsg, nLen);
    }

    //Connect to the destination island
    strncpy(pstShip->psLocation, pstDest->sName, sizeof(pstShip->psLocation) - 1);
    pstShip->psLocation[sizeof(pstShip->psLocation) - 1] = '\0';
    return 0;
}