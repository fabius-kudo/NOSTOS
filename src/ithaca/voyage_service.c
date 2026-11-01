#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "voyage_service.h"

// Prints ">>> <name> <text>\n" in a single write so threads don't interleave lines.
static void logEvent(const char *psName, const char *psText) {
    char *psLog = NULL;
    int nLen = asprintf(&psLog, ">>> %s %s\n", psName, psText);

    if (nLen >= 0) {
        write(STDOUT_FILENO, psLog, nLen);
        free(psLog);
    }
}

static void handleConnect(tClientSession *pstSession, const tFrame *pstFrame) {
    char sName[FRAME_DATA_SIZE + 1]; 
    
    if (pstFrame->ucDataLength == 0) {
        sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_ERR, "EMPTY_NAME");
        return;                     // stay connected, let them retry
    }
    if (pstSession->nIdentified) {
        sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_ERR, "ALREADY_CONNECTED");
        return;
    }

    // frameDataToString always NUL-terminates and truncates to the buffer size
    frameDataToString(pstFrame, sName, sizeof(sName));      // local buffer, always NUL-terminated
    pstSession->psName = dupString(sName);
    if (pstSession->psName == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }
    pstSession->nIdentified = 1;

    sendTextFrame(pstSession->nFd, TYPE_CONNECT_ITHACA, FLAGS_OK, "OK");
    logEvent(pstSession->psName, "connected to Ithaca.");
}

static void handleDisconnect(tClientSession *pstSession) {
    if (!pstSession->nIdentified) {
        sendTextFrame(pstSession->nFd, TYPE_DISCONNECT, FLAGS_ERR, "NOT_CONNECTED");
        return;
    }

    logEvent(pstSession->psName, "has left Ithaca. Fair winds!");
    free(pstSession->psName);            // replaces: pstSession->sName[0] = '\0';
    pstSession->psName = NULL;
    pstSession->nIdentified = 0;

    sendTextFrame(pstSession->nFd, TYPE_DISCONNECT, FLAGS_OK, "OK");
}

static void handleListVoyages(tClientSession *pstSession) {
    tVoyageStore *pstStore = pstSession->pstStore;
    tVoyage *pstAvail;
    char sData[FRAME_DATA_SIZE + 1];
    int nTotal = 0;
    int nLen;

    pstAvail = malloc(sizeof(tVoyage) * (pstStore->nNumVoyages > 0 ? pstStore->nNumVoyages : 1));
    if (pstAvail == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }

    // Copy the available voyages under the lock, send them after releasing it.
    // The copies are shallow: they share the store's strings, which are not freed while running.
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
                i + 1, nTotal, pstAvail[i].nId, pstAvail[i].psObject,
                pstAvail[i].psDestination, pstAvail[i].nReward);
        if (nLen < 0 || nLen > FRAME_DATA_SIZE ||
            sendFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_OK, sData, nLen) < 0) {
            break;
        }
    }
    free(pstAvail);

    logEvent(pstSession->psName, "requested available voyages.");
}

static void handleAccept(tClientSession *pstSession, const tFrame *pstFrame) {
    tVoyageStore *pstStore = pstSession->pstStore;
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    const char *psError = NULL;
    tVoyage *pstVoyage = NULL;
    int nId, nLen = 0;

    // Get the voyage ID from the frame data
    frameDataToString(pstFrame, sData, sizeof(sData));
    nId = atoi(sData);
    if (sData[0] == '\0' || nId <= 0) {
        sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "INVALID_ID");
        return;
    }

    pthread_mutex_lock(&pstStore->stMutex);

    // The loop must not stop at the match: it also has to check that this ship
    // doesn't already hold another voyage, which needs a pass over all of them.
    for (int i = 0; i < pstStore->nNumVoyages; i++) {
        tVoyage *pstV = &pstStore->pstVoyages[i];

        if (pstV->eState == VOYAGE_ASSIGNED && pstV->sOwner != NULL &&
            strcmp(pstV->sOwner, pstSession->psName) == 0) {
            psError = "ALREADY_HAVE_VOYAGE";
            break;
        }
        if (pstV->nId == nId) {
            pstVoyage = pstV;
        }
    }

    if (psError == NULL) {
        if (pstVoyage == NULL) {
            psError = "VOYAGE_NOT_FOUND";
        } else if (pstVoyage->eState != VOYAGE_AVAILABLE) {
            psError = "VOYAGE_NOT_AVAILABLE";
        } else {
            char *psOwner = dupString(pstSession->psName);
            if (psOwner == NULL) {
                psError = "INTERNAL_ERROR";
            } else {
                free(pstVoyage->sOwner);
                pstVoyage->sOwner = psOwner;
                pstVoyage->eState = VOYAGE_ASSIGNED;

                nLen = snprintf(sReply, sizeof(sReply), "OK&%d&%s&%s&%d",
                                pstVoyage->nId, pstVoyage->psObject,
                                pstVoyage->psDestination, pstVoyage->nReward);

                if (nLen < 0 || nLen > FRAME_DATA_SIZE) {      // undo
                    pstVoyage->eState = VOYAGE_AVAILABLE;
                    free(pstVoyage->sOwner);
                    pstVoyage->sOwner = NULL;
                    psError = "INTERNAL_ERROR";
                }
            }
        }
    }

    pthread_mutex_unlock(&pstStore->stMutex);

    if (psError != NULL) {
        sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, psError);
        return;
    }

    sendFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_OK, sReply, nLen);

    char *psLog = NULL;
    int nLogLen = asprintf(&psLog, ">>> Voyage %d assigned to %s.\n", nId, pstSession->psName);
    if (nLogLen >= 0) {
        write(STDOUT_FILENO, psLog, nLogLen);
        free(psLog);
    }
}

void serveClient(tClientSession *pstSession) {
    tFrame stFrame;

    while (1) {
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
                handleConnect(pstSession, &stFrame);
                break;
            case TYPE_LIST_VOYAGES:
                handleListVoyages(pstSession);
                break;
            case TYPE_ACCEPT:
                handleAccept(pstSession, &stFrame);
                break;
            case TYPE_DISCONNECT:
                handleDisconnect(pstSession);
                break;
            default:
                sendTextFrame(pstSession->nFd, TYPE_NACK, FLAGS_ERR, "INVALID_TYPE");
                break;
        }
    }
}