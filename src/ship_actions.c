#include "ship_actions.h"
#include "ship_config.h"
#include "protocol.h"
#include "socket_utils.h"

static int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]) {
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

static int cmdListVoyages(tShip *pstShip, int nArgc, char *ppsArgv[]) {
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

        char *psIndex  = strtok(sData, "&");
        char *psTotal  = strtok(NULL, "&");

        if (psTotal == NULL) {
            write(STDOUT_FILENO, "No voyages available.\n", strlen("No voyages available.\n"));
            return 0;
        }
        char *psId     = strtok(NULL, "&");
        char *psObject = strtok(NULL, "&");
        char *psDest   = strtok(NULL, "&");
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

static int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void) nArgc;

    int nLen;
    char *sMsg;
    tFrame stReply;
    char sData[FRAME_DATA_SIZE + 1];
    char *psSave = NULL;
    char *psOk, *psId, *psObject, *psDest, *psReward;

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

    psOk = strtok_r(sData, "&", &psSave);
    psId = strtok_r(NULL, "&", &psSave);
    psObject = strtok_r(NULL, "&", &psSave);
    psDest = strtok_r(NULL, "&", &psSave);
    psReward = strtok_r(NULL, "&", &psSave);
    if (!psOk || !psId || !psObject || !psDest || !psReward) {
        write(STDOUT_FILENO, "Error: malformed reply from Ithaca.\n", strlen("Error: malformed reply from Ithaca.\n"));
        return -1;
    }

    pstShip->stVoyage.nId = atoi(psId);
    strncpy(pstShip->stVoyage.sOrigin, psObject, sizeof(pstShip->stVoyage.sOrigin) - 1);
    pstShip->stVoyage.sOrigin[sizeof(pstShip->stVoyage.sOrigin) - 1] = '\0';
    strncpy(pstShip->stVoyage.sDestination, psDest, sizeof(pstShip->stVoyage.sDestination) - 1);
    pstShip->stVoyage.sDestination[sizeof(pstShip->stVoyage.sDestination) - 1] = '\0';
    pstShip->stVoyage.nReward = atoi(psReward);
    pstShip->nHasVoyage = 1;

    nLen = snprintf(sMsg, sizeof(sMsg), "Voyage %d accepted.\nDestination: %s.\n", pstShip->stVoyage.nId, pstShip->stVoyage.sDestination);
    write(STDOUT_FILENO, sMsg, nLen);

    return 0;
}

static int cmdSail(tShip *pstShip, int nArgc, char *ppsArgv[]) {

    //Get the destination from the command arguments

    //Check if the destination is valid and reachable from the current location

    //Diconnect from Ithaca/current island

    //Check if the island has space to dock the ship

    //Connect to the destination island

}