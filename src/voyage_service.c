#include "voyage_service.h"

static int handleConnect(tClientSession *pstSession, const tFrame *pstFrame) {
    char sName[FRAME_DATA_SIZE + 1];
    char sLog[200];
    int nLen;

    if (pstFrame->ucDataLength == 0) {
        sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_ERR, "EMPTY_NAME");
        return 0;                   // stay connected, let them retry
    }
    if (pstSession->nIdentified) {
        sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_ERR, "ALREADY_CONNECTED");
        return 0;
    }

    frameDataToString(pstFrame, sName, sizeof(sName));

    strncpy(pstSession->sName, sName, sizeof(pstSession->sName) - 1);
    pstSession->sName[sizeof(pstSession->sName) - 1] = '\0';
    pstSession->nIdentified = 1;

    sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_OK, "OK");

    nLen = snprintf(sLog, sizeof(sLog), ">>> %s connected to Ithaca.\n", pstSession->sName);
    write(STDOUT_FILENO, sLog, nLen);
    return 0;
}

//TODO: make that only still available voyages are listed, not the ones that have been accepted by other clients
void handleListVoyages(tClientSession *pstSession) {
    char sData[FRAME_DATA_SIZE + 1];
    int nLen;
    
    //TODO: maybe we dont need mutex lock, was autocomplete
    pthread_mutex_lock(&pstSession->pstStore->stMutex);

    for (int i = 0; i < pstSession->pstStore->nNumVoyages; i++) {
        tVoyage *pstVoyage = &pstSession->pstStore->pstVoyages[i];

        nLen = snprintf(sData, sizeof(sData), "%d. %s -> %s [%d]\n",
                        pstVoyage->nId,
                        pstVoyage->psOrigin,
                        pstVoyage->psDestination,
                        pstVoyage->nReward);

        if (nLen < 0 || nLen > FRAME_DATA_SIZE) {
            continue;
        }
        if (sendFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, sData, nLen) < 0) {
            pthread_mutex_unlock(&pstSession->pstStore->stMutex);
            return;
        }
    }

    // Terminator: a frame with no data
    sendFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, NULL, 0);

    pthread_mutex_unlock(&pstSession->pstStore->stMutex);
}

void handleAccept(tClientSession *pstSession, const tFrame *pstFrame) {
    //Get the voyage ID from the frame data

    //Check if the voyage ID is valid and available

    //If valid, mark the voyage as accepted and send a success response

    //If not valid, send an error response

}

void serveClient(tClientSession *pstSession) {
    tFrame stFrame;
    int nKeepGoing = 1;

    while (nKeepGoing) {
        int nRet = recvFrame(pstSession->nFd, &stFrame);
        if (nRet <= 0) {
            break;                  // 0 = peer closed/crashed, -1 = error
        }

        if (stFrame.ucFlags != FLAG_REQUEST) {
            sendTextFrame(pstSession->nFd, TYPE_NACK, FLAGS_ERR, "INVALID_FLAGS");
            continue;
        }

        // Everything except CONNECT requires being identified first
        if (stFrame.ucType != TYPE_CONNECT_ITHACA && !pstSession->nIdentified) {
            sendTextFrame(pstSession->nFd, stFrame.ucType, FLAGS_ERR, "NOT_CONNECTED");
            continue;
        }

        switch (stFrame.ucType) {
            case TYPE_CONNECT_ITHACA:
                nKeepGoing = (handleConnect(pstSession, &stFrame) == 0);
                break;
            case TYPE_LIST_VOYAGES:
                handleListVoyages(pstSession);
                break;
            case TYPE_ACCEPT:
                handleAccept(pstSession, &stFrame);
                break;
            case TYPE_DISCONNECT:
                handleDisconnect(pstSession);
                nKeepGoing = 0;
                break;
            default:
                sendTextFrame(pstSession->nFd, TYPE_NACK, FLAGS_ERR, "INVALID_TYPE");
                break;
        }
    }
}