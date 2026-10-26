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
    int nCount = 0;

    if (pstShip->nIthacaFd < 0) {
        write(STDOUT_FILENO, "Error: cannot reach Ithaca.\n", strlen("Error: cannot reach Ithaca.\n"));
        return -1;
    }

    sendFrame(pstShip->nIthacaFd, TYPE_LIST_VOYAGES, FLAG_REQUEST, NULL, 0);

    while (1) {
        if (recvFrame(pstShip->nIthacaFd, &stFrame) != 1) {
            write(STDOUT_FILENO, "Error: connection to Ithaca lost.\n", strlen("Error: connection to Ithaca lost.\n"));
            return -1;
        }

        if (stFrame.ucDataLength == 0) {
            break;
        }

        if (nCount == 0) {
            write(STDOUT_FILENO, "--- AVAILABLE VOYAGES ---\n", strlen("--- AVAILABLE VOYAGES ---\n"));
        }
        write(STDOUT_FILENO, stFrame.aucData, stFrame.ucDataLength);
        nCount++;
    }

    if (nCount == 0) {
        write(STDOUT_FILENO, "No voyages available.\n", strlen("No voyages available.\n"));
    }
    return 0;
}

static int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]) {
    (void) nArgc;
    (void) ppsArgv;

    if (pstShip->nIthacaFd < 0) {
        write(STDOUT_FILENO, "Error: cannot reach Ithaca.\n", strlen("Error: cannot reach Ithaca.\n"));
        return -1;
    }

    //TODO: implement sending voyage acceptance to Ithaca and handling the response
}