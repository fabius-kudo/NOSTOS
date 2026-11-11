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
 * @Name: leavePort
 * @Def: Takes the ship out of the port, whatever its state, and writes on
 *       the island screen what happened.
 * @Arg: In/Out: pstSession session of the ship
 * @Ret: None.
 *
 ***********************************************/
void leavePort(tIslandSession *pstSession) {
    char *psNext = NULL;
    char *psMsg = NULL;
    tPortState eBefore;

    // If the ship was docked, its dock is handed to the oldest waiting ship,
    // which has already been sent DOCKED when this call returns
    eBefore = portLeave(pstSession->pstPort, &pstSession->stShip, &psNext);

    if (eBefore == PORT_DOCKED) {
        if (asprintf(&psMsg, ">>> %s has left.\n", pstSession->psShipName) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
    } else if (eBefore == PORT_QUEUED) {
        if (asprintf(&psMsg, ">>> %s was lost while waiting; removed from the FIFO queue.\n", pstSession->psShipName) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
    }

    if (psNext != NULL) {
        if (asprintf(&psMsg, ">>> DOCKED sent to %s; he may enter the port.\n", psNext) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
        free(psNext);
    }
}

/***********************************************
 *
 * @Name: handleArrive
 * @Def: Handles the arrival of a ship: it docks at once if there is a free
 *       dock, otherwise it is added to the FIFO list of waiting ships.
 * @Arg: In/Out: pstSession session of the ship
 *       In: pstFrame received arrival frame (data is the ship name)
 * @Ret: Returns 0 to keep serving the ship, -1 to end its session.
 *
 ***********************************************/
int handleArrive(tIslandSession *pstSession, const tFrame *pstFrame) {
    char sName[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;
    int nRes;

    if (portGetState(pstSession->pstPort, &pstSession->stShip) != PORT_OUT) {
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

    // portArrive takes a free dock or adds the ship to the waiting list, and
    // sends the answer (DOCKED or WAIT) itself
    nRes = portArrive(pstSession->pstPort, &pstSession->stShip, pstSession->psShipName);
    if (nRes < 0) {
        // The ship is gone: serveShip takes it out of the port
        return -1;
    }
    if (nRes == 1) {
        if (asprintf(&psMsg, ">>> %s entered the port.\n", pstSession->psShipName) < 0) {
            psMsg = NULL;
        }
        printAndFree(psMsg);
        return 0;
    }

    // Port full. The thread does not sleep: it goes back to reading the
    // socket, and the ship that frees a dock will send the DOCKED
    if (asprintf(&psMsg, ">>> Port full. %s waiting offshore in the FIFO queue.\n", pstSession->psShipName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
    return 0;
}

/***********************************************
 *
 * @Name: handleLeave
 * @Def: Handles a docked ship leaving the port and frees its dock.
 * @Arg: In/Out: pstSession session of the ship
 * @Ret: Returns 0 to keep serving the ship, -1 to end its session.
 *
 ***********************************************/
int handleLeave(tIslandSession *pstSession) {
    if (portGetState(pstSession->pstPort, &pstSession->stShip) != PORT_DOCKED) {
        sendTextFrame(pstSession->nFd, TYPE_ISLAND_LEAVE, FLAGS_ERR, "NOT_DOCKED");
        return 0;
    }

    // Gives the dock to the oldest waiting ship
    leavePort(pstSession);
    sendTextFrame(pstSession->nFd, TYPE_ISLAND_LEAVE, FLAGS_OK, "OK");
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
void handleBuyMap(tIslandSession *pstSession, const tFrame *pstFrame) {
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;
    int nLen;

    if (portGetState(pstSession->pstPort, &pstSession->stShip) != PORT_DOCKED) {
        sendTextFrame(pstSession->nFd, TYPE_BUY_MAP, FLAGS_ERR, "NOT_DOCKED");
        return;
    }

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
void handleListMarket(tIslandSession *pstSession) {
    char sData[FRAME_DATA_SIZE + 1];
    tProduct *pstList;
    int nNum = 0;
    int nTotal;
    int nLen;

    if (portGetState(pstSession->pstPort, &pstSession->stShip) != PORT_DOCKED) {
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
 * @Name: sendTradeError
 * @Def: Tells the ship why its purchase or sale was rejected.
 * @Arg: In: pstSession session of the ship
 *       In: ucType type of the request (TYPE_BUY or TYPE_SELL)
 *       In: eRes result of the stock operation
 * @Ret: None.
 *
 ***********************************************/
void sendTradeError(tIslandSession *pstSession, unsigned char ucType, tStockResult eRes) {
    switch (eRes) {
        case STOCK_BAD_AMOUNT:
            sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "INVALID_AMOUNT");
            break;
        case STOCK_NO_PRODUCT:
            sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "PRODUCT_NOT_FOUND");
            break;
        case STOCK_NOT_ENOUGH:
            sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "NOT_ENOUGH_STOCK");
            break;
        default:
            sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "INTERNAL_ERROR");
            break;
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
int parseTrade(const tFrame *pstFrame, char *psData, int nSize, char **ppsProduct, int *pnAmount) {
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
void handleTrade(tIslandSession *pstSession, const tFrame *pstFrame, int nIsBuy) {
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

    if (portGetState(pstSession->pstPort, &pstSession->stShip) != PORT_DOCKED) {
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
        sendTradeError(pstSession, ucType, eRes);
        return;
    }

    nLen = snprintf(sReply, sizeof(sReply), "OK&%s&%d&%d", sCanonical, nAmount, nMoney);
    if (nLen < 0 || nLen > FRAME_DATA_SIZE) {
        sendTextFrame(pstSession->nFd, ucType, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }
    sendFrame(pstSession->nFd, ucType, FLAGS_OK, sReply, nLen);

    if (nIsBuy) {
        nLen = asprintf(&psMsg, ">>> %s bought %d kg of %s.\n", pstSession->psShipName, nAmount, sCanonical);
    } else {
        nLen = asprintf(&psMsg, ">>> %s sold %d kg of %s.\n", pstSession->psShipName, nAmount, sCanonical);
    }
    if (nLen < 0) {
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
    int nEnd = 0;

    while (!nEnd) {
        // Docked and waiting ships are both read here, so a ship that
        // disconnects is noticed at once, also while it waits for a dock.
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

    // A ship that vanishes while docked must free its dock, and one that
    // vanishes while waiting must leave the list. Nothing happens here if it
    // already left with handleLeave or never arrived
    leavePort(pstSession);
    free(pstSession->psShipName);
    pstSession->psShipName = NULL;
}