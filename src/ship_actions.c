#define _GNU_SOURCE
#include "ship_actions.h"

/* ---------- small output helpers ---------- */

static void say(const char *psMsg) {
    write(STDOUT_FILENO, psMsg, strlen(psMsg));
}

// Writes a heap string to stdout and frees it. Does nothing for NULL.
static void sayAndFree(char *psMsg) {
    if (psMsg != NULL) {
        write(STDOUT_FILENO, psMsg, strlen(psMsg));
        free(psMsg);
    }
}

static void sayError(const char *psReason) {
    char *psMsg = NULL;
    if (asprintf(&psMsg, "Error: %s\n", psReason) < 0) {
        psMsg = NULL;                       // contents are undefined when asprintf fails
    }
    sayAndFree(psMsg);
}

/* ---------- ship state helpers ---------- */

// Replaces the ship's location with a heap copy. Returns 0 on success, -1 on malloc failure.
static int setLocation(tShip *pstShip, const char *psName) {
    char *psCopy = dupString(psName);
    if (psCopy == NULL) {
        return -1;
    }
    free(pstShip->psLocation);              // free(NULL) is fine on the first call
    pstShip->psLocation = psCopy;
    return 0;
}

// Replaces the active voyage. Returns 0 on success, -1 on malloc failure.
static int setVoyage(tShip *pstShip, int nId, const char *psObject, const char *psDest, int nReward) {
    char *psObjCopy = dupString(psObject);
    char *psDestCopy = dupString(psDest);

    if (psObjCopy == NULL || psDestCopy == NULL) {
        free(psObjCopy);
        free(psDestCopy);
        return -1;
    }
    free(pstShip->stVoyage.psObject);
    free(pstShip->stVoyage.psDestination);
    pstShip->stVoyage.nId = nId;
    pstShip->stVoyage.psObject = psObjCopy;
    pstShip->stVoyage.psDestination = psDestCopy;
    pstShip->stVoyage.nReward = nReward;
    pstShip->nHasVoyage = 1;
    return 0;
}

// Closes the connections and frees everything the ship owns, including its config.
void releaseShip(tShip *pstShip) {
    closeSocket(&pstShip->nIthacaFd);
    closeSocket(&pstShip->nIslandFd);

    free(pstShip->psLocation);
    free(pstShip->stVoyage.psObject);
    free(pstShip->stVoyage.psDestination);

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

    freeOdysseus(&pstShip->stConfig);
    memset(pstShip, 0, sizeof(*pstShip));
    pstShip->nIthacaFd = -1;
    pstShip->nIslandFd = -1;
}

/* ---------- commands ---------- */

int cmdConnectIthaca(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void)nArgc;
    (void)ppsArgv;

    tFrame stReply;
    char sData[FRAME_DATA_SIZE + 1];

    if (pstShip->nIthacaFd >= 0) {
        say("Already connected to Ithaca\n");
        return 0;
    }

    int nFd = connectToServer(pstShip->stConfig.psIthacaIp, pstShip->stConfig.nIthacaPort);
    if (nFd < 0) {
        say("Error: cannot reach Ithaca.\n");
        return -1;
    }

    if (sendFrame(nFd, TYPE_CONNECT_ITHACA, FLAG_REQUEST,
                  pstShip->stConfig.psName, (int)strlen(pstShip->stConfig.psName)) != 0
        || recvFrame(nFd, &stReply) != 1) {
        say("Error: connection to Ithaca lost.\n");
        closeSocket(&nFd);
        return -1;
    }
    if (stReply.ucFlags != FLAGS_OK) {
        frameDataToString(&stReply, sData, sizeof(sData));
        sayError(sData);                    // the reason sent by Ithaca
        closeSocket(&nFd);
        return -1;
    }

    if (setLocation(pstShip, "Ithaca") < 0) {
        say("Error: out of memory.\n");
        closeSocket(&nFd);
        return -1;
    }
    pstShip->nIthacaFd = nFd;
    say("Connected to Ithaca.\n");
    return 0;
}

int cmdListVoyages(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void)nArgc;
    (void)ppsArgv;

    tFrame stFrame;
    char sData[FRAME_DATA_SIZE + 1];

    if (pstShip->nIthacaFd < 0) {
        say("Error: not connected to Ithaca.\n");
        return -1;
    }
    if (pstShip->nHasVoyage) {
        say("Error: you already have an active voyage.\n");
        return -1;
    }

    if (sendFrame(pstShip->nIthacaFd, TYPE_LIST_VOYAGES, FLAG_REQUEST, NULL, 0) != 0) {
        say("Error: connection to Ithaca lost.\n");
        closeSocket(&pstShip->nIthacaFd);
        return -1;
    }

    int nIndex = 0;
    int nTotal = 1;
    while (nIndex < nTotal) {
        if (recvFrame(pstShip->nIthacaFd, &stFrame) != 1) {
            say("Error: connection to Ithaca lost.\n");
            closeSocket(&pstShip->nIthacaFd);
            return -1;
        }
        frameDataToString(&stFrame, sData, sizeof(sData));

        if (stFrame.ucFlags != FLAGS_OK) {              // error: data is the reason
            sayError(sData);
            return -1;
        }

        char *psIndex = strtok(sData, "&");
        char *psTotal = strtok(NULL, "&");

        if (psTotal == NULL) {                          // Ithaca sends just "0" when empty
            say("No voyages available.\n");
            return 0;
        }
        char *psId = strtok(NULL, "&");
        char *psObject = strtok(NULL, "&");
        char *psDest = strtok(NULL, "&");
        char *psReward = strtok(NULL, "&");

        if (!psIndex || !psId || !psObject || !psDest || !psReward) {
            say("Error: malformed reply from Ithaca.\n");
            return -1;
        }

        nIndex = atoi(psIndex);
        nTotal = atoi(psTotal);
        if (nIndex <= 0 || nTotal <= 0) {
            say("Error: malformed reply from Ithaca.\n");
            return -1;
        }
        if (nIndex == 1) {
            say("--- AVAILABLE VOYAGES ---\n");
        }

        char *psLine = NULL;
        if (asprintf(&psLine, "%2s. %-22s -> %-10s [%s]\n", psId, psObject, psDest, psReward) < 0) {
            psLine = NULL;
        }
        sayAndFree(psLine);
    }
    return 0;
}

int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void)nArgc;

    tFrame stReply;
    char sData[FRAME_DATA_SIZE + 1];

    if (pstShip->nIthacaFd < 0) {
        say("Error: not connected to Ithaca.\n");
        return -1;
    }
    if (pstShip->nHasVoyage) {
        say("Error: you already have an active voyage.\n");
        return -1;
    }

    if (sendFrame(pstShip->nIthacaFd, TYPE_ACCEPT, FLAG_REQUEST,
                  ppsArgv[0], (int)strlen(ppsArgv[0])) != 0
        || recvFrame(pstShip->nIthacaFd, &stReply) != 1) {
        say("Error: connection to Ithaca lost.\n");
        closeSocket(&pstShip->nIthacaFd);
        return -1;
    }
    frameDataToString(&stReply, sData, sizeof(sData));

    if (stReply.ucFlags != FLAGS_OK) {
        sayError(sData);                    // the reason sent by Ithaca
        return -1;
    }

    // OK&<VoyageID>&<Object>&<Destination>&<Reward>
    char *psOk = strtok(sData, "&");
    char *psId = strtok(NULL, "&");
    char *psObject = strtok(NULL, "&");
    char *psDest = strtok(NULL, "&");
    char *psReward = strtok(NULL, "&");
    if (!psOk || !psId || !psObject || !psDest || !psReward) {
        say("Error: malformed reply from Ithaca.\n");
        return -1;
    }

    if (setVoyage(pstShip, atoi(psId), psObject, psDest, atoi(psReward)) < 0) {
        say("Error: out of memory.\n");
        return -1;
    }

    char *psMsg = NULL;
    if (asprintf(&psMsg, "Voyage %d accepted.\nDestination: %s.\n",
                 pstShip->stVoyage.nId, pstShip->stVoyage.psDestination) < 0) {
        psMsg = NULL;
    }
    sayAndFree(psMsg);
    return 0;
}

/* ---------- sailing ---------- */

static const tKnownIsland *findKnownIsland(const tShip *pstShip, const char *psName) {
    for (int i = 0; i < pstShip->nNumIslands; i++) {
        if (strcasecmp(pstShip->pstIslands[i].sName, psName) == 0) {
            return &pstShip->pstIslands[i];
        }
    }
    return NULL;
}

static int isDirectRoute(const tShip *pstShip, const char *psFrom, const char *psTo) {
    for (int i = 0; i < pstShip->nNumRoutes; i++) {
        const tKnownRoute *pstR = &pstShip->pstRoutes[i];
        if ((strcasecmp(pstR->sA, psFrom) == 0 && strcasecmp(pstR->sB, psTo) == 0) ||
            (strcasecmp(pstR->sA, psTo) == 0 && strcasecmp(pstR->sB, psFrom) == 0)) {
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
        return;                             // already at sea
    }

    if (sendFrame(*pnFd, ucType, FLAG_REQUEST, NULL, 0) == 0) {
        recvFrame(*pnFd, &stReply);         // wait for the confirmation; we leave either way
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

static void consumeProvisions(tShip *pstShip, int nKg) {
    for (int i = 0; i < pstShip->stConfig.nNumFoodItems && nKg > 0; i++) {
        tFood *pstFood = &pstShip->stConfig.pstFoodSupplies[i];
        int nTake = (pstFood->nQuantity < nKg) ? pstFood->nQuantity : nKg;
        pstFood->nQuantity -= nTake;
        nKg -= nTake;
    }
}

static long nowMs(void) {
    struct timespec stTs;
    clock_gettime(CLOCK_MONOTONIC, &stTs);
    return stTs.tv_sec * 1000L + stTs.tv_nsec / 1000000L;
}

static int waitForDocking(tShip *pstShip, const char *psIsland) {
    struct pollfd astFds[1];
    tFrame stFrame;
    char sData[FRAME_DATA_SIZE + 1];
    char *sMsg;
    int nLen;

    long nNextRation = nowMs() + RATION_INTERVAL_MS;

    astFds[0].fd = pstShip->nIslandFd;
    astFds[0].events = POLLIN;
    
    while (1) {
        long nRemaining = nNextRation - nowMs();
        if (nRemaining < 0) {
            nRemaining = 0;
        }

        astFds[0].revents = 0;
        int nRet = poll(astFds, 1, (int)nRemaining);

        if (nRet < 0) {
            // EINTR = a signal arrived (CTRL+C); anything else is a real error
            closeSocket(&pstShip->nIslandFd);
            return 0;
        }

        if (nRet > 0) {
            if (recvFrame(pstShip->nIslandFd, &stFrame) != 1) {
                write(STDOUT_FILENO, "Error: connection to the island lost.\n", strlen("Error: connection to the island lost.\n"));
                closeSocket(&pstShip->nIslandFd);
                return 0;
            }
            frameDataToString(&stFrame, sData, sizeof(sData));
            if (stFrame.ucType == TYPE_ISLAND_ARRIVE && stFrame.ucFlags == FLAGS_OK && strcmp(sData, "DOCKED") == 0) {
                nLen = asprintf(&sMsg, "\n%s has granted access to the port.\n", psIsland);
                if (nLen < 0){
                    write(STDOUT_FILENO, "asprint error\n", strlen("asprint error\n"));
                } else {
                    write(STDOUT_FILENO, sMsg, nLen);
                }
                return 1;
            }
            continue;
        }
            
        consumeProvisions(pstShip, RATION_KG);
        nNextRation += RATION_INTERVAL_MS;

        nLen = asprintf(&sMsg, "\n>>> The crew consumes another ration.\nProvisions remaining: %d kg\n", totalProvisions(pstShip));
        if (nLen < 0){
            write(STDOUT_FILENO, "asprint error\n", strlen("asprint error\n"));
        } else {
            write(STDOUT_FILENO, sMsg, nLen);
        }

        if (totalProvisions(pstShip) <= 0) {
            nLen = asprintf(&sMsg, "\nThe crew has run out of provisions.\nOdysseus %s has been lost at sea.\n", pstShip->stConfig.psName);
            if (nLen < 0){
                write(STDOUT_FILENO, "asprint error\n", strlen("asprint error\n"));
            } else {
                write(STDOUT_FILENO, sMsg, nLen);
            }
            closeSocket(&pstShip->nIslandFd);
            return 0;
        }
    }
}

int cmdSail(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void)nArgc;

    tFrame stReply;
    char sData[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;
    int nFd;

    if (pstShip->psLocation == NULL || pstShip->psLocation[0] == '\0') {
        say("Error: you are not at any port.\n");
        return -1;
    }

    int nFromIthaca = (strcasecmp(pstShip->psLocation, "Ithaca") == 0);
    const tKnownIsland *pstDest = findKnownIsland(pstShip, ppsArgv[0]);

    // From Ithaca only the starting island is valid; from an island, only direct routes
    int nReachable = 0;
    if (pstDest != NULL) {
        if (nFromIthaca) {
            nReachable = (strcasecmp(pstDest->sName, pstShip->stConfig.psIslandName) == 0);
        } else {
            nReachable = isDirectRoute(pstShip, pstShip->psLocation, pstDest->sName);
        }
    }
    if (!nReachable) {
        if (asprintf(&psMsg, "We can't sail to %s from here...\n", ppsArgv[0]) < 0) {
            psMsg = NULL;
        }
        sayAndFree(psMsg);
        return -1;
    }

    if (asprintf(&psMsg, "Leaving %s...\nSailing to %s...\n", pstShip->psLocation, pstDest->sName) < 0) {
        psMsg = NULL;
    }
    sayAndFree(psMsg);
    leaveCurrentPort(pstShip);

    nFd = connectToServer(pstDest->sIp, pstDest->nPort);
    if (nFd < 0) {
        psMsg = NULL;
        if (asprintf(&psMsg, "Error: cannot reach %s (%s).\n", pstDest->sName, strerror(errno)) < 0) {
            psMsg = NULL;
        }
        sayAndFree(psMsg);
        return -1;                          // at sea; psLocation still holds the last port
    }

    if (sendTextFrame(nFd, TYPE_ISLAND_ARRIVE, FLAG_REQUEST, pstShip->stConfig.psName) != 0
        || recvFrame(nFd, &stReply) != 1) {
        say("Error: the island did not answer.\n");
        closeSocket(&nFd);
        return -1;
    }
    frameDataToString(&stReply, sData, sizeof(sData));
    if (stReply.ucFlags != FLAGS_OK) {      // data is the reason
        sayError(sData);
        closeSocket(&nFd);
        return -1;
    }

    pstShip->nIslandFd = nFd;

    psMsg = NULL;
    if (asprintf(&psMsg, "\nConnected to %s.\n", pstDest->sName) < 0) {
        psMsg = NULL;
    }
    sayAndFree(psMsg);

    if (strcmp(sData, "WAIT") == 0) {
        psMsg = NULL;
        if (asprintf(&psMsg, "Waiting off the coast of %s...\n", pstDest->sName) < 0) {
            psMsg = NULL;
        }
        sayAndFree(psMsg);
        say("The harbor is full. We drop anchor and wait.\n");
        if (!waitForDocking(pstShip, pstDest->sName)) {
            return -1;
        }
    } else {
        psMsg = NULL;
        if (asprintf(&psMsg, "%s has granted access to the port.\n", pstDest->sName) < 0) {
            psMsg = NULL;
        }
        sayAndFree(psMsg);
    }

    if (setLocation(pstShip, pstDest->sName) < 0) {
        say("Error: out of memory.\n");
        return -1;
    }
    return 0;
}

static int addKnownIsland(tShip *pstShip, const char *psName, const char *psIp, int nPort) {
    if (findKnownIsland(pstShip, psName) != NULL) {
        return 0;
    }

    tKnownIsland *pstNew = realloc(pstShip->pstIslands, sizeof(tKnownIsland) * (pstShip->nNumIslands + 1));
    if (pstNew == NULL) {
        return -1;
    }
    pstShip->pstIslands = pstNew;

    tKnownIsland *pstIsland = &pstShip->pstIslands[pstShip->nNumIslands];
    pstIsland->sName = dupString(psName);
    pstIsland->sIp = dupString(psIp);
    pstIsland->nPort = nPort;
    if (pstIsland->sName == NULL || pstIsland->sIp == NULL) {
        free(pstIsland->sName);
        free(pstIsland->sIp);
        return -1;
    }

    pstShip->nNumIslands++;
    return 0;
}

static int addKnownRoute(tShip *pstShip, const char *psA, const char *psB) {
    if (isDirectRoute(pstShip, psA, psB)) {
        return 0;
    }

    tKnownRoute *pstNew = realloc(pstShip->pstRoutes, sizeof(tKnownRoute) * (pstShip->nNumRoutes + 1));
    if (pstNew == NULL) {
        return -1;
    }
    pstShip->pstRoutes = pstNew;

    tKnownRoute *pstRoute = &pstShip->pstRoutes[pstShip->nNumRoutes];
    pstRoute->sA = dupString(psA);
    pstRoute->sB = dupString(psB);
    if (pstRoute->sA == NULL || pstRoute->sB == NULL) {
        free(pstRoute->sA);
        free(pstRoute->sB);
        return -1;
    }

    pstShip->nNumRoutes++;
    return 1;
}

int cmdBuyMap(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    tFrame stFrame;
    char sData[FRAME_DATA_SIZE + 1];
    int nIndex = 0, nTotal = 1, nCharged = 0, nRoutes = 0;

    if (nArgc != 1 || atoi(ppsArgv[0]) != 1) {
        say("Error: only one map can be bought at a time (BUY MAP 1).\n");
        return -1;
    }
    if (pstShip->nIslandFd < 0) {
        say("Error: you are not docked at an island.\n");
        return -1;
    }
    if (pstShip->nMoney < MAP_PRICE) {
        say("Error: not enough gold.\n");
        return -1;
    }

    if (sendFrame(pstShip->nIslandFd, TYPE_BUY_MAP, FLAG_REQUEST, "1", 1) != 0) {
        say("Error: connection to the island lost.\n");
        closeSocket(&pstShip->nIslandFd);
        return -1;
    }

    while (nIndex < nTotal) {
        if (recvFrame(pstShip->nIslandFd, &stFrame) != 1) {
            say("Error: connection to the island lost.\n");
            closeSocket(&pstShip->nIslandFd);
            return -1;
        }
        frameDataToString(&stFrame, sData, sizeof(sData));
        if (stFrame.ucFlags != FLAGS_OK) {
            sayError(sData);
            return -1;
        }

        // OK&50&Index&Total&Name&IP&Port   (or OK&50&0 when there are no routes)
        char *psOk = strtok(sData, "&");
        char *psPrice = strtok(NULL, "&");
        char *psIndex = strtok(NULL, "&");
        char *psTotal = strtok(NULL, "&");
        if (!psOk || !psPrice || !psIndex) {
            say("Error: malformed reply from the island.\n");
            return -1;
        }

        if (!nCharged) {                            // charge once, on the first OK frame
            pstShip->nMoney -= atoi(psPrice);
            nCharged = 1;
        }
        if (psTotal == NULL) {                      // "OK&50&0": the map has no routes
            break;
        }

        char *psName = strtok(NULL, "&");
        char *psIp = strtok(NULL, "&");
        char *psPort = strtok(NULL, "&");
        if (!psName || !psIp || !psPort) {
            say("Error: malformed reply from the island.\n");
            return -1;
        }

        nIndex = atoi(psIndex);
        nTotal = atoi(psTotal);
        if (nIndex <= 0 || nTotal <= 0) {
            say("Error: malformed reply from the island.\n");
            return -1;
        }

        if (addKnownIsland(pstShip, psName, psIp, atoi(psPort)) < 0) {
            say("Error: out of memory.\n");
            return -1;
        }
        int nAdded = addKnownRoute(pstShip, pstShip->psLocation, psName);
        if (nAdded < 0) {
            say("Error: out of memory.\n");
            return -1;
        }
        nRoutes += nAdded; 
    }

    char *psMsg = NULL;
    if (asprintf(&psMsg, "Bought the map of %s for %d gold.\n%s", pstShip->psLocation, MAP_PRICE, nRoutes > 0 ? "New routes have been charted.\n" : "") < 0) {
        psMsg = NULL;
    }
    sayAndFree(psMsg);
    return 0;
}