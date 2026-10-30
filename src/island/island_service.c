#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "island_service.h"
#include "custom_dynamic.h"

// One write per line so threads don't interleave output.
static void logEvent(const char *psFormat, const char *psA, const char *psB) {
    char *psLog = NULL;
    int nLen = asprintf(&psLog, psFormat, psA, psB);

    if (nLen >= 0) {
        write(STDOUT_FILENO, psLog, nLen);
        free(psLog);
    }
}

// Returns 0 to keep serving, -1 to end the session.
static int handleArrive(tIslandSession *pstSession, const tFrame *pstFrame) {
    char sName[FRAME_DATA_SIZE + 1];

    if (pstSession->eState != SHIP_NONE) {
        sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_ERR, "ALREADY_ARRIVED");
        return 0;
    }
    if (pstFrame->ucDataLength == 0) {
        sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_ERR, "EMPTY_NAME");
        return 0;
    }
    frameDataToString(pstFrame, sName, sizeof(sName));
    pstSession->psShipName = dupString(sName);
    if (pstSession->psShipName == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_ERR, "INTERNAL_ERROR");
        return -1;
    }

    if (portArrive(pstSession->pstPort)) {
        pstSession->eState = SHIP_DOCKED;
        if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "OK") != 0) {
            return -1;                  // serveShip releases the dock
        }
        logEvent(">>> %s docked at %s.\n", pstSession->psShipName, pstSession->psIslandName);
        return 0;
    }

    // Port full: tell the ship to wait, then block until our ticket comes up
    pstSession->eState = SHIP_QUEUED;
    if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "WAIT") != 0) {
        // The ticket is already taken and must be consumed, or the queue stalls
        if (portWaitTurn(pstSession->pstPort) == 0) {
            portLeave(pstSession->pstPort);
        }
        pstSession->eState = SHIP_GONE;
        return -1;
    }
    logEvent(">>> %s is waiting to enter %s.\n", pstSession->psShipName, pstSession->psIslandName);

    if (portWaitTurn(pstSession->pstPort) != 0) {
        pstSession->eState = SHIP_GONE; // port closing
        return -1;
    }
    if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "OK") != 0) {
        portLeave(pstSession->pstPort); // the ship died while queued: pass the dock on
        pstSession->eState = SHIP_GONE;
        return -1;
    }
    pstSession->eState = SHIP_DOCKED;
    logEvent(">>> %s docked at %s.\n", pstSession->psShipName, pstSession->psIslandName);
    return 0;
}

static int handleLeave(tIslandSession *pstSession) {
    if (pstSession->eState != SHIP_DOCKED) {
        sendTextFrame(pstSession->nFd, TYPE_ISLAND_LEAVE, FLAGS_ERR, "NOT_DOCKED");
        return 0;
    }
    portLeave(pstSession->pstPort);
    pstSession->eState = SHIP_GONE;
    sendTextFrame(pstSession->nFd, TYPE_ISLAND_LEAVE, FLAGS_OK, "OK");
    logEvent(">>> %s left %s.\n", pstSession->psShipName, pstSession->psIslandName);
    return -1;
}

static void handleBuyMap(tIslandSession *pstSession, const tFrame *pstFrame) {
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    int nLen;

    frameDataToString(pstFrame, sData, sizeof(sData));
    if (strcmp(sData, "1") != 0) {
        sendTextFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_ERR, "INVALID_AMOUNT");
        return;
    }

    tIsland *pstIsland = pstSession->pstIsland;
    int nTotal = pstIsland->nNumRoutes;

    if (nTotal == 0) {
        sendTextFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_OK, "OK&50&0");
        return;
    }
    for (int i = 0; i < nTotal; i++) {
        nLen = snprintf(sReply, sizeof(sReply), "OK&50&%d&%d&%s&%s&%d", i + 1, nTotal,
                        pstIsland->pstRoutes[i].psName, pstIsland->pstRoutes[i].psIp,
                        pstIsland->pstRoutes[i].nPort);
        if (nLen < 0 || nLen > FRAME_DATA_SIZE || sendFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_OK, sReply, nLen) < 0) {
            return;
        }
    }
}

void serveShip(tIslandSession *pstSession) {
    tFrame stFrame;
    int nEnd = 0;

    while (!nEnd) {
        if (recvFrame(pstSession->nFd, &stFrame) <= 0) {
            break;                      // peer closed or crashed
        }
        if (stFrame.ucFlags != FLAG_REQUEST) {
            sendTextFrame(pstSession->nFd, TYPE_NACK, FLAGS_ERR, "INVALID_FLAGS");
            continue;
        }
        switch (stFrame.ucType) {
            case TYPE_ISLAND_ARRIVE:
                nEnd = (handleArrive(pstSession, &stFrame) < 0);
                break;
            case TYPE_ISLAND_LEAVE:
                nEnd = (handleLeave(pstSession) < 0);
                break;
            case TYPE_BUY_MAP:
                handleBuyMap(pstSession, &stFrame);
                break;
            default:
                sendTextFrame(pstSession->nFd, TYPE_NACK, FLAGS_ERR, "INVALID_TYPE");
                break;
        }
    }

    // A ship that vanishes while docked must free its dock
    if (pstSession->eState == SHIP_DOCKED) {
        portLeave(pstSession->pstPort);
        pstSession->eState = SHIP_GONE;
    }
    free(pstSession->psShipName);
    pstSession->psShipName = NULL;
}