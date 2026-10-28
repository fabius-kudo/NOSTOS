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

void handleListVoyages(tClientSession *pstSession) {
    tVoyageStore *pstStore = pstSession->pstStore;
    tVoyage *pstAvail;
    char sData[FRAME_DATA_SIZE + 1];
    int nTotal = 0;
    int nLen;

    pstAvail = malloc(sizeof(tVoyage) * (pstStore->nNumVoyages > 0 ? pstStore->nNumVoyages : 1));
    if (pstAvail == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_ERR, "INTERNAL_ERROR");
        return -1;
    }
    
    pthread_mutex_lock(&pstStore->stMutex);
    for (int i = 0; i < pstStore->nNumVoyages; i++) {
        if (pstStore->pstVoyages[i].eState == VOYAGE_AVAILABLE) {
            pstAvail[nTotal++] = pstStore->pstVoyages[i];
        }
    }
    pthread_mutex_unlock(&pstStore->stMutex);

    if (nTotal == 0) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, "0");
    }
    for (int i = 0; i < nTotal; i++) {
        nLen = snprintf(sData, sizeof(sData), "%d&%d&%d&%s&%s&%d",
                        i + 1, nTotal, pstAvail[i].nId, pstAvail[i].psOrigin, pstAvail[i].psDestination, pstAvail[i].nReward);
        if (nLen < 0 || nLen > FRAME_DATA_SIZE ||
            sendFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, sData, nLen) < 0) {
            break;
        }
    }
    free(pstAvail);
    return 0;
}

void handleAccept(tClientSession *pstSession, const tFrame *pstFrame) {
    tVoyageStore *pstStore = pstSession->pstStore;
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    const char *psError = NULL;
    tVoyage *pstVoyage = NULL;
    int nId, nLen = 0;

    //Get the voyage ID from the frame data
    frameDataToString(pstFrame, sData, sizeof(sData));
    nId = atoi(sData);
    if (sData[0] == '\0' || nId <= 0) {
        sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "INVALID_ID");
        return;
    }

    //Check if the voyage ID is valid and available
    pthread_mutex_lock(&pstStore->stMutex);

    for (int i = 0; i < pstStore->nNumVoyages; i++) {
        tVoyage *pstV = &pstStore->pstVoyages[i];

        if (pstV->eState == VOYAGE_ASSIGNED && strcmp(pstV->sOwner, pstSession->sName) == 0) {
            psError = "ALREADY_HAVE_VOYAGE";     // this ship still holds one
            break;
        }
        if (pstV->nId == nId) {
            pstVoyage = pstV;
            //TODO: idk if we can stop loop here
        }
    }

    if (psError == NULL) {
        if (pstVoyage == NULL) {
            psError = "VOYAGE_NOT_FOUND";
        } else if (pstVoyage->eState != VOYAGE_AVAILABLE) {
            psError = "VOYAGE_NOT_AVAILABLE";
        } else {
            pstVoyage->eState = VOYAGE_ASSIGNED;
            strncpy(pstVoyage->sOwner, pstSession->sName, sizeof(pstVoyage->sOwner) - 1);
            pstVoyage->sOwner[sizeof(pstVoyage->sOwner) - 1] = '\0';

            // Build the reply while the data is protected
            nLen = snprintf(sReply, sizeof(sReply), "OK&%d&%s&%s&%d", 
                    pstVoyage->nId, pstVoyage->psOrigin, pstVoyage->psDestination, pstVoyage->nReward);
        }
    }

    pthread_mutex_unlock(&pstStore->stMutex);

    //If not valid, send an error response
    if (psError != NULL) {
        sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, psError);
        return;
    }    
    
    //If valid, mark the voyage as accepted and send a success response

    //TODO: idk if we need this check, but just in case
    if (nLen < 0 || nLen > FRAME_DATA_SIZE) {
        // Reply doesn't fit in a frame: undo the assignment
        pthread_mutex_lock(&pstStore->stMutex);
        pstVoyage->eState = VOYAGE_AVAILABLE;
        pstVoyage->sOwner[0] = '\0';
        pthread_mutex_unlock(&pstStore->stMutex);
        sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }

    sendFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_OK, sReply, nLen);
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