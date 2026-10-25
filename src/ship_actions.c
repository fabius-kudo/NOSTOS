#include "ship_actions.h"
#include "ship_config.h"
#include "protocol.h"
#include "socket_utils.h"

static int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]) {
    (void) pstShip;
    (void) argc;
    (void) argv;

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
    sendFrame(nFd, 0x01, 0x00, pstShip->stConfig.psName, strlen(pstShip->stConfig.psName));

    //TODO: make ithaca respond
    tFrame stReply;
    if (recvFrame(nFd, &stReply) <= 0 || stReply.ucFlags != 0x80) {
        write(STDOUT_FILENO, "Error: Ithaca refused the connection.\n", strlen("Error: Ithaca refused the connection.\n"));
        close(nFd);
        return -1;
    }

    pstShip->nIthacaFd = nFd;
    strcpy(pstShip->psLocation, "Ithaca");
    write(STDOUT_FILENO, "Connected to Ithaca...\n", strlen("Connected to Ithaca...\n"));
    return 0;
}

static int cmdListVoyages(tShip *pContext, int nArgc, char *ppsArgv[]) {
    tFrame stFrame;

    if (pContext->nIthacaFd < 0) { /* print error, return -1 */ }

    sendFrame(pContext->nIthacaFd, TYPE_LIST_VOYAGES, FLAG_REQUEST, NULL, 0);

    do {
        if (recvFrame(pContext->nIthacaFd, &stFrame) != 1) { /* connection lost */ }
        // parse Index&Total&ID&Object&Destination&Reward and print it
    } while (/* index < total */);
    return 0;
}