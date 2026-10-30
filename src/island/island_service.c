#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "island_service.h"
#include "custom_dynamic.h"
#include "port_config.h"

#define MAP_PRICE 50

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
    if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "DOCKED") != 0) {
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

static void handleListMarket(tIslandSession *pstSession) {
    char sData[FRAME_DATA_SIZE + 1];
    tProduct *pstList;
    int nNum = 0;
    int nTotal;
    int nLen;

    if (pstSession->eState != SHIP_DOCKED) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_ERR, "NOT_DOCKED");
        return;
    }

    // Copy under the stock lock; the frames are sent after it is released
    pstList = stockSnapshot(pstSession->pstStock, &nNum);
    if (pstList == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }

    nTotal = nNum + 1;                      // every product plus the map entry
    for (int i = 0; i < nNum; i++) {
        nLen = snprintf(sData, sizeof(sData), "%d&%d&%s&%d&%d",
                        i + 1, nTotal, pstList[i].sName, pstList[i].nQuantity, pstList[i].nPrice);
        if (nLen < 0 || nLen > FRAME_DATA_SIZE ||
            sendFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_OK, sData, nLen) != 0) {
            free(pstList);
            return;                         // the ship is gone: serveShip notices on the next read
        }
    }
    nLen = snprintf(sData, sizeof(sData), "%d&%d&MAP&1&%d", nTotal, nTotal, MAP_PRICE);
    sendFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_OK, sData, nLen);
    free(pstList);
}

static const char *stockReason(tStockResult eRes) {
    switch (eRes) {
        case STOCK_BAD_AMOUNT:  return "INVALID_AMOUNT";
        case STOCK_NO_PRODUCT:  return "PRODUCT_NOT_FOUND";
        case STOCK_NOT_ENOUGH:  return "NOT_ENOUGH_STOCK";
        default:                return "INTERNAL_ERROR";
    }
}

// Shared by BUY and SELL: both requests are "<Product>&<Amount>". Returns 0 or -1.
static int parseTrade(const tFrame *pstFrame, char *psData, int nSize, char **ppsProduct, int *pnAmount) {
    char *psAmount;

    frameDataToString(pstFrame, psData, nSize);
    *ppsProduct = strtok(psData, "&");
    psAmount = strtok(NULL, "&");
    if (*ppsProduct == NULL || psAmount == NULL) {
        return -1;
    }
    *pnAmount = atoi(psAmount);                 // 0 or negative is rejected by the stock
    return 0;
}

static void logTrade(const char *psShip, const char *psVerb, int nAmount, const char *psProduct) {
    char *psLog = NULL;
    int nLen = asprintf(&psLog, ">>> %s %s %d kg of %s.\n", psShip, psVerb, nAmount, psProduct);

    if (nLen >= 0) {
        write(STDOUT_FILENO, psLog, nLen);
        free(psLog);
    }
}

static void handleTrade(tIslandSession *pstSession, const tFrame *pstFrame, int nIsBuy) {
    unsigned char ucType = nIsBuy ? TYPE_BUY : TYPE_SELL;
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    char sCanonical[MAX_NAME];
    char *psProduct = NULL;
    int nAmount = 0;
    int nMoney = 0;
    int nLen;
    tStockResult eRes;

    if (pstSession->eState != SHIP_DOCKED) {
        sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "NOT_DOCKED");
        return;
    }
    if (parseTrade(pstFrame, sData, sizeof(sData), &psProduct, &nAmount) < 0) {
        sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "INVALID_FORMAT");
        return;
    }

    if (nIsBuy) {
        eRes = stockBuy(pstSession->pstStock, psProduct, nAmount, &nMoney, sCanonical);
    } else {
        eRes = stockSell(pstSession->pstStock, psProduct, nAmount, &nMoney, sCanonical);
    }
    if (eRes != STOCK_OK) {
        sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, stockReason(eRes));
        return;
    }

    nLen = snprintf(sReply, sizeof(sReply), "OK&%s&%d&%d", sCanonical, nAmount, nMoney);
    if (nLen < 0 || nLen > FRAME_DATA_SIZE) {
        sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }
    sendFrame(pstSession->nFd, ucType, FLAGS_OK, sReply, nLen);
    logTrade(pstSession->psShipName, nIsBuy ? "bought" : "sold", nAmount, sCanonical);
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
            case TYPE_LIST_MARKET:
                handleListMarket(pstSession);
                break;
            case TYPE_BUY:
                handleTrade(pstSession, &stFrame, 1);
                break;
            case TYPE_SELL:
                handleTrade(pstSession, &stFrame, 0);
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