/***********************************************
 *
 * @File : voyage_service.c
 * @Purpose : Server-side handling of the Odysseus <-> Ithaca protocol.
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 1/11/26
 *
 ***********************************************/

#define _GNU_SOURCE
#include "voyage_service.h"
#include "custom_io.h"
#include "file_transfer.h"


/***********************************************
 *
 * @Name: handleConnect
 * @Def: Handles CONNECT (0x01). Stores the Odysseus name sent in the frame,
 *       marks the session as identified and replies OK. Replies with an
 *       error frame if the name is empty or the client already identified
 *       itself; the connection stays open in both cases.
 * @Arg: In/Out: pstSession session of the requesting client
 *       In: pstFrame received request frame
 * @Ret: None.
 *
 ***********************************************/
void handleConnect(tClientSession *pstSession, const tFrame *pstFrame) {
    char sName[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;

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
    if (asprintf(&psMsg, ">>> %s connected to Ithaca.\n", pstSession->psName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
}

/***********************************************
 *
 * @Name: handleDisconnect
 * @Def: Handles DISCONNECT (0x05), sent by an Odysseus before setting sail
 *       or closing. Forgets the client's identity and confirms with OK, so
 *       the Odysseus can close the socket. Any voyage assigned to it stays
 *       assigned while the ship is at sea.
 * @Arg: In/Out: pstSession session of the requesting client
 * @Ret: None.
 *
 ***********************************************/
void handleDisconnect(tClientSession *pstSession) {
    char *psMsg = NULL;

    if (!pstSession->nIdentified) {
        sendTextFrame(pstSession->nFd, TYPE_DISCONNECT, FLAGS_ERR, "NOT_CONNECTED");
        return;
    }

    if (asprintf(&psMsg, ">>> %s has left Ithaca. Fair winds!\n", pstSession->psName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
    free(pstSession->psName);
    pstSession->psName = NULL;
    pstSession->nIdentified = 0;

    sendTextFrame(pstSession->nFd, TYPE_DISCONNECT, FLAGS_OK, "OK");
}

/***********************************************
 *
 * @Name: handleListVoyages
 * @Def: Handles LIST VOYAGES (0x02). Sends one frame per available voyage
 *       in the format Index&Total&ID&Object&Destination&Reward, or a single
 *       "0" frame if none are available. Before listing, any voyage still
 *       assigned to this Odysseus is marked as failed: it can only happen
 *       when the ship was lost at sea and restarted, as an Odysseus with an
 *       active voyage never asks for the list.
 * @Arg: In: pstSession session of the requesting client
 * @Ret: None.
 *
 ***********************************************/
void handleListVoyages(tClientSession *pstSession) {
    tVoyageStore *pstStore = pstSession->pstStore;
    tVoyage *pstAvail;
    char sData[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;
    int nTotal = 0;
    int nLen;

    pstAvail = malloc(sizeof(tVoyage) * (pstStore->nNumVoyages > 0 ? pstStore->nNumVoyages : 1));
    if (pstAvail == NULL) {
        sendTextFrame(pstSession->nFd, TYPE_LIST_VOYAGES, FLAGS_ERR, "INTERNAL_ERROR");
        return;
    }

    // Copy the available voyages under the lock, send them after releasing it.
    pthread_mutex_lock(&pstStore->stMutex);
    for (int i = 0; i < pstStore->nNumVoyages; i++) {
        tVoyage *pstV = &pstStore->pstVoyages[i];
        // A restarted ship that still owns a voyage lost it at sea is marked as failed
        if (pstV->eState == VOYAGE_ASSIGNED && pstV->sOwner != NULL && strcmp(pstV->sOwner, pstSession->psName) == 0) {
            pstV->eState = VOYAGE_FAILED;
        }
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

    if (asprintf(&psMsg, ">>> %s requested available voyages.\n", pstSession->psName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
}

/***********************************************
 *
 * @Name: sendAcceptError
 * @Def: Tells the Odysseus why its ACCEPT request was rejected.
 * @Arg: In: pstSession session of the requesting client
 *       In: eRes reason of the rejection
 * @Ret: None.
 *
 ***********************************************/
void sendAcceptError(tClientSession *pstSession, tAcceptResult eRes) {
    switch (eRes) {
        case ACCEPT_ALREADY_HAVE_VOYAGE:
            sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "ALREADY_HAVE_VOYAGE");
            break;
        case ACCEPT_NOT_FOUND:
            sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "VOYAGE_NOT_FOUND");
            break;
        case ACCEPT_NOT_AVAILABLE:
            sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "VOYAGE_NOT_AVAILABLE");
            break;
        case ACCEPT_NO_OBJECT:
            sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "OBJECT_NOT_AVAILABLE");
            break;
        default:
            sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "INTERNAL_ERROR");
            break;
    }
}

/***********************************************
 *
 * @Name: findVoyage
 * @Def: Looks up a voyage by its identifier. The identifier and the file of
 *       a voyage never change after startup, so they can be read without
 *       the lock; its state and owner can only be read with the lock.
 * @Arg: In: pstStore voyage store
 *       In: nId voyage identifier
 * @Ret: Pointer to the voyage, or NULL if it does not exist.
 *
 ***********************************************/
tVoyage *findVoyage(tVoyageStore *pstStore, int nId) {
    for (int i = 0; i < pstStore->nNumVoyages; i++) {
        if (pstStore->pstVoyages[i].nId == nId) {
            return &pstStore->pstVoyages[i];
        }
    }
    return NULL;
}

/***********************************************
 *
 * @Name: objectPath
 * @Def: Builds the path of the object file of a voyage. The file named in
 *       voyages.dat is used if it can be opened as written; otherwise it is
 *       looked for by name in the objects folder of the Ithaca config.
 * @Arg: In: pstStore voyage store (holds the objects folder)
 *       In: pstVoyage voyage whose object is needed
 * @Ret: Heap copy of the path (the caller frees it), or NULL on error.
 *
 ***********************************************/
char *objectPath(const tVoyageStore *pstStore, const tVoyage *pstVoyage) {
    char *psPath = NULL;
    const char *psName = strrchr(pstVoyage->psFile, '/');
    int nFd = open(pstVoyage->psFile, O_RDONLY);

    if (nFd >= 0) {
        close(nFd);
        return dupString(pstVoyage->psFile);
    }
    psName = (psName == NULL) ? pstVoyage->psFile : psName + 1;
    if (asprintf(&psPath, "%s/%s", pstStore->psObjectsDir, psName) < 0) {
        return NULL;
    }
    return psPath;
}

/***********************************************
 *
 * @Name: releaseVoyage
 * @Def: Makes a voyage available again after its object could not be
 *       delivered to the Odysseus that accepted it.
 * @Arg: In/Out: pstStore voyage store
 *       In/Out: pstVoyage voyage to release
 *       In: psOwner name of the Odysseus it was assigned to
 * @Ret: None.
 *
 ***********************************************/
void releaseVoyage(tVoyageStore *pstStore, tVoyage *pstVoyage, const char *psOwner) {
    pthread_mutex_lock(&pstStore->stMutex);
    if (pstVoyage->eState == VOYAGE_ASSIGNED && pstVoyage->sOwner != NULL &&
        strcmp(pstVoyage->sOwner, psOwner) == 0) {
        pstVoyage->eState = VOYAGE_AVAILABLE;
        free(pstVoyage->sOwner);
        pstVoyage->sOwner = NULL;
    }
    pthread_mutex_unlock(&pstStore->stMutex);
}

/***********************************************
 *
 * @Name: handleAccept
 * @Def: Handles ACCEPT (0x03). Assigns the requested voyage to this Odysseus
 *       if it exists, is still available, and the ship holds no other
 *       assigned voyage. The check and the assignment happen under the same
 *       lock, so two ships can never get the same voyage. Replies
 *       OK&ID&Object&Destination&Reward, or an error frame with the reason,
 *       and then sends the object file (OBJECT transfer). If the object
 *       cannot be delivered, the voyage becomes available again.
 * @Arg: In: pstSession session of the requesting client
 *       In: pstFrame received request frame (data = voyage ID)
 * @Ret: None.
 *
 ***********************************************/
void handleAccept(tClientSession *pstSession, const tFrame *pstFrame) {
    tVoyageStore *pstStore = pstSession->pstStore;
    char sData[FRAME_DATA_SIZE + 1];
    char sReply[FRAME_DATA_SIZE + 1];
    char *psMsg = NULL;
    tAcceptResult eRes = ACCEPT_OK;
    tVoyage *pstVoyage = NULL;
    char sMd5[MD5_LENGTH + 1];
    char *psPath = NULL;
    int nSize = 0;
    int nId, nLen = 0;
    int nRes;

    //Get the voyage ID from the frame data
    frameDataToString(pstFrame, sData, sizeof(sData));
    nId = atoi(sData);
    if (sData[0] == '\0' || nId <= 0) {
        sendTextFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_ERR, "INVALID_ID");
        return;
    }

    // The object is checked before taking the lock (md5sum runs a child
    // process): if it cannot be sent, the voyage is not assigned at all
    pstVoyage = findVoyage(pstStore, nId);
    if (pstVoyage != NULL) {
        psPath = objectPath(pstStore, pstVoyage);
        if (psPath == NULL || prepareFile(psPath, &nSize, sMd5) != 0) {
            free(psPath);
            sendAcceptError(pstSession, ACCEPT_NO_OBJECT);
            return;
        }
    }
    pstVoyage = NULL;

    pthread_mutex_lock(&pstStore->stMutex);

    //The loop doesnt stop at the match, it also has to check that this ship doesn't already hold another voyage
    for (int i = 0; i < pstStore->nNumVoyages; i++) {
        tVoyage *pstV = &pstStore->pstVoyages[i];

        if (pstV->eState == VOYAGE_ASSIGNED && pstV->sOwner != NULL &&
            strcmp(pstV->sOwner, pstSession->psName) == 0) {
            eRes = ACCEPT_ALREADY_HAVE_VOYAGE;
            break;
        }
        if (pstV->nId == nId) {
            pstVoyage = pstV;
        }
    }

    if (eRes == ACCEPT_OK) {
        if (pstVoyage == NULL) {
            eRes = ACCEPT_NOT_FOUND;
        } else if (pstVoyage->eState != VOYAGE_AVAILABLE) {
            eRes = ACCEPT_NOT_AVAILABLE;
        } else {
            char *psOwner = dupString(pstSession->psName);
            if (psOwner == NULL) {
                eRes = ACCEPT_INTERNAL_ERROR;
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
                    eRes = ACCEPT_INTERNAL_ERROR;
                }
            }
        }
    }

    pthread_mutex_unlock(&pstStore->stMutex);

    // The answer is sent after releasing the lock
    if (eRes != ACCEPT_OK) {
        free(psPath);
        sendAcceptError(pstSession, eRes);
        return;
    }

    if (sendFrame(pstSession->nFd, TYPE_ACCEPT, FLAGS_OK, sReply, nLen) != 0) {
        releaseVoyage(pstStore, pstVoyage, pstSession->psName);
        free(psPath);
        return;
    }

    if (asprintf(&psMsg, ">>> Voyage %d assigned to %s.\n>>> Sending %s to %s...\n",
                 nId, pstSession->psName, pstVoyage->psObject, pstSession->psName) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);

    // OBJECT transfer. The lock is not held: it can take a while, and the
    // voyage is already assigned to this ship, so nobody else can take it
    nRes = sendFile(pstSession->nFd, "OBJECT", psPath, nSize, sMd5, 1);
    free(psPath);

    if (nRes == TRANSFER_OK) {
        printF(">>> Object transferred successfully.\n");
        return;
    }
    // The Odysseus does not have the object, so the voyage is not its
    releaseVoyage(pstStore, pstVoyage, pstSession->psName);
    if (asprintf(&psMsg, ">>> The object could not be sent to %s. Voyage %d is available again.\n",
                 pstSession->psName, nId) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
}

/***********************************************
 *
 * @Name: serveClient
 * @Def: Serves one Odysseus until it disconnects: reads request frames in a
 *       loop and dispatches them by type. Frames that are not requests, or
 *       of an unknown type, are answered with a NACK frame. Every request
 *       except CONNECT requires the client to be identified first.
 * @Arg: In/Out: pstSession session of the client to serve
 * @Ret: None. The caller closes the socket and frees the session.
 *
 ***********************************************/
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

    //Free crashed client
    free(pstSession->psName);
    pstSession->psName = NULL;
    pstSession->nIdentified = 0;
}