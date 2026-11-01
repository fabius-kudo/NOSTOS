/***********************************************
 *
 * @File : island_service.c
 * @Purpose : Attends one ship at a time on the island (docking, market and map)
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "island_service.h"

#define MAP_PRICE 50

/***********************************************
 *
 * @Name: handleArrive
 * @Def: Handles the arrival of a ship: it docks at once if there is a free
 *       berth, otherwise it waits in the FIFO queue until one is handed to it.
 * @Arg: In/Out: pstSession session of the ship
 *       In: pstFrame received arrival frame (data is the ship name)
 * @Ret: Returns 0 to keep serving the ship, -1 to end its session.
 *
 ***********************************************/
static int handleArrive(tIslandSession *pstSession, const tFrame *pstFrame) {
    char sName[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;

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

    if (asprintf(&psMsg, ">>> %s approaching %s.\n", pstSession->psShipName, pstSession->psIslandName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);

    // portArrive takes a free berth or puts the ship in the queue
    if (portArrive(pstSession->pstPort)) {
        pstSession->eState = SHIP_DOCKED;
        if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "DOCKED") != 0) {
            //ship is gone
            return -1;
        }
        if (asprintf(&psMsg, ">>> %s entered the port.\n", pstSession->psShipName) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
        return 0;
    }

    // Port full: tell the ship to wait, sleep
    pstSession->eState = SHIP_QUEUED;
    if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "WAIT") != 0) {
        // The place in the queue is already counted: it must be used and given back
        if (portWaitTurn(pstSession->pstPort) == 0) {
            portLeave(pstSession->pstPort);
        }
        pstSession->eState = SHIP_GONE;
        return -1;
    }
    if (asprintf(&psMsg, ">>> Port full. %s waiting offshore in the FIFO queue.\n", pstSession->psShipName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);

    // Sleeps on the semaphore of the port
    if (portWaitTurn(pstSession->pstPort) != 0) {
        pstSession->eState = SHIP_GONE;
        return -1;
    }
    if (sendTextFrame(pstSession->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "DOCKED") != 0) {
        // The ship died while queued: pass the berth on to the next one
        portLeave(pstSession->pstPort);
        pstSession->eState = SHIP_GONE;
        return -1;
    }
    pstSession->eState = SHIP_DOCKED;
    if (asprintf(&psMsg, ">>> DOCKED sent to %s; he may enter the port.\n", pstSession->psShipName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
    return 0;
}

/***********************************************
 *
 * @Name: handleLeave
 * @Def: Handles a docked ship leaving the port and frees its berth.
 * @Arg: In/Out: pstSession session of the ship
 * @Ret: Returns 0 to keep serving the ship, -1 to end its session.
 *
 ***********************************************/
static int handleLeave(tIslandSession *pstSession) {
    char *psMsg = NULL;

    if (pstSession->eState != SHIP_DOCKED) {
        sendTextFrame(pstSession->nFd, TYPE_ISLAND_LEAVE, FLAGS_ERR, "NOT_DOCKED");
        return 0;
    }

    // Gives the berth to the oldest queued ship
    portLeave(pstSession->pstPort);
    pstSession->eState = SHIP_GONE;
    sendTextFrame(pstSession->nFd, TYPE_ISLAND_LEAVE, FLAGS_OK, "OK");
    if (asprintf(&psMsg, ">>> %s has left.\n", pstSession->psShipName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
    return -1;
}

/***********************************************
 *
 * @Name: handleBuyMap
 * @Def: Sends the island routes to a ship that buys the local map.
 * @Arg: In/Out: pstSession session of the ship
 *       In: pstFrame received frame (data is the amount, only 1 is valid)
 * @Ret: None.
 *
 ***********************************************/
static void handleBuyMap(tIslandSession *pstSession, const tFrame *pstFrame) {
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;
    int nLen;

    frameDataToString(pstFrame, sData, sizeof(sData));
    if (strcmp(sData, "1") != 0) {
        sendTextFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_ERR, "INVALID_AMOUNT");
        return;
    }

    // The routes never change after startup, so they are read without a lock
    const tIsland *pstIsland = pstSession->pstIsland;
    int nTotal = pstIsland->nNumRoutes;

    if (nTotal == 0) {
        sendTextFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_OK, "OK&50&0");
        if (asprintf(&psMsg, ">>> %s bought the local map.\n", pstSession->psShipName) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
        return;
    }

    // One frame for each route
    for (int i = 0; i < nTotal; i++) {
        nLen = snprintf(sReply, sizeof(sReply), "OK&50&%d&%d&%s&%s&%d", i + 1, nTotal,
                        pstIsland->pstRoutes[i].psName, pstIsland->pstRoutes[i].psIpAddress,
                        pstIsland->pstRoutes[i].nPort);
        if (nLen < 0 || nLen > FRAME_DATA_SIZE || sendFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_OK, sReply, nLen) < 0) {
            return;
        }
    }
}

/***********************************************
 *
 * @Name: handleListMarket
 * @Def: Sends the products of the island and the price of the map to a docked ship.
 * @Arg: In/Out: pstSession session of the ship
 * @Ret: None.
 *
 ***********************************************/
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

    // The copy is made under the stock lock; the frames are sent after it is released
    pstList = stockSnapshot(pstSession->pstStock, &nNum);
    if (pstList == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }

    // Every product plus the map entry
    nTotal = nNum + 1;
    for (int i = 0; i < nNum; i++) {
        nLen = snprintf(sData, sizeof(sData), "%d&%d&%s&%d&%d",
                        i + 1, nTotal, pstList[i].sName, pstList[i].nQuantity, pstList[i].nPrice);
        if (nLen < 0 || nLen > FRAME_DATA_SIZE ||
            sendFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_OK, sData, nLen) != 0) {
            // The ship is gone: serveShip notices it on the next read
            free(pstList);
            return;
        }
    }
    nLen = snprintf(sData, sizeof(sData), "%d&%d&MAP&1&%d", nTotal, nTotal, MAP_PRICE);
    sendFrame(pstSession->nFd, TYPE_LIST_MARKET, FLAGS_OK, sData, nLen);
    free(pstList);
}

/***********************************************
 *
 * @Name: stockReason
 * @Def: Converts a stock result into the reason text sent to the ship.
 * @Arg: In: eRes result of a stock operation
 * @Ret: Returns the reason string.
 *
 ***********************************************/
static const char *stockReason(tStockResult eRes) {
    switch (eRes) {
        case STOCK_BAD_AMOUNT:  return "INVALID_AMOUNT";
        case STOCK_NO_PRODUCT:  return "PRODUCT_NOT_FOUND";
        case STOCK_NOT_ENOUGH:  return "NOT_ENOUGH_STOCK";
        default:                return "INTERNAL_ERROR";
    }
}

/***********************************************
 *
 * @Name: parseTrade
 * @Def: Splits the data of a BUY or SELL request: "<Product>&<Amount>".
 * @Arg: In: pstFrame received frame
 *       Out: psData buffer where the data is copied
 *       In: nSize size of the buffer
 *       Out: ppsProduct pointer to the product name inside the buffer
 *       Out: pnAmount pointer to store the amount
 * @Ret: Returns 0 on success, -1 if the format is wrong.
 *
 ***********************************************/
static int parseTrade(const tFrame *pstFrame, char *psData, int nSize, char **ppsProduct, int *pnAmount) {
    char *psAmount;

    frameDataToString(pstFrame, psData, nSize);
    *ppsProduct = strtok(psData, "&");
    psAmount = strtok(NULL, "&");
    if (*ppsProduct == NULL || psAmount == NULL) {
        return -1;
    }

    // A zero or negative amount is rejected later by the stock
    *pnAmount = atoi(psAmount);
    return 0;
}

/***********************************************
 *
 * @Name: handleTrade
 * @Def: Handles a purchase or a sale of a product by a docked ship.
 * @Arg: In/Out: pstSession session of the ship
 *       In: pstFrame received frame
 *       In: nIsBuy 1 for a purchase, 0 for a sale
 * @Ret: None.
 *
 ***********************************************/
static void handleTrade(tIslandSession *pstSession, const tFrame *pstFrame, int nIsBuy) {
    unsigned char ucType = nIsBuy ? TYPE_BUY : TYPE_SELL;
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    char sCanonical[MAX_NAME];
    char *psProduct = NULL;
    char *psMsg = NULL;
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

    // The stock locks its own mutex inside these calls
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

    if (asprintf(&psMsg, ">>> %s %s %d kg of %s.\n", pstSession->psShipName,
                 nIsBuy ? "bought" : "sold", nAmount, sCanonical) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
}

/***********************************************
 *
 * @Name: serveShip
 * @Def: Main loop of a ship thread: reads its requests and answers them
 *       until the ship leaves or the connection is lost.
 * @Arg: In/Out: pstSession session of the ship
 * @Ret: None.
 *
 ***********************************************/
void serveShip(tIslandSession *pstSession) {
    tFrame stFrame;
    char *psMsg = NULL;
    int nEnd = 0;

    while (!nEnd) {
        // 0 = the ship closed the connection or crashed, -1 = error
        if (recvFrame(pstSession->nFd, &stFrame) <= 0) {
            break;
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

    // A ship that vanishes while docked must free its berth
    if (pstSession->eState == SHIP_DOCKED) {
        portLeave(pstSession->pstPort);
        pstSession->eState = SHIP_GONE;
        if (asprintf(&psMsg, ">>> %s has left.\n", pstSession->psShipName) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
    }
    free(pstSession->psShipName);
    pstSession->psShipName = NULL;
}