/***********************************************
 *
 * @File : island_main.c
 * @Purpose : Main function for the Island application
 *
 ***********************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

#include "port_config.h"
#include "port_control.h"
#include "island_service.h"
#include "socket_utils.h"
#include "island_stock.h"
#include "custom_io.h"

static volatile sig_atomic_t gnStop = 0;

void handleExit(int nSig) {
    (void)nSig;
    gnStop = 1;
}

static void *shipThread(void *pArg) {
    sigset_t stSet;
    tIslandSession *pstSession = (tIslandSession *)pArg;

    sigemptyset(&stSet);
    sigaddset(&stSet, SIGINT);
    pthread_sigmask(SIG_BLOCK, &stSet, NULL);   // only the main thread handles Ctrl+C

    serveShip(pstSession);
    closeSocket(&pstSession->nFd);
    free(pstSession);
    return NULL;
}

int main(int argc, char *argv[]) {
    int nNumProducts = 0;
    int nLen = 0;
    int nServerFd = -1;
    char *psBuf = NULL;
    char *psErr = NULL;
    tProduct *pstStock = NULL;
    tIsland stIsland;
    tPort stPort;
    tStock stStock;
    struct sigaction stAct;

    if (argc != 3) {
        printF("Usage: ./island <config.dat> <stock.db>\n");
        exit(EXIT_FAILURE);
    }

    memset(&stAct, 0, sizeof(stAct));
    stAct.sa_handler = handleExit;
    sigemptyset(&stAct.sa_mask);
    stAct.sa_flags = 0;                         // no SA_RESTART: accept() returns EINTR
    sigaction(SIGINT, &stAct, NULL);
    signal(SIGPIPE, SIG_IGN);

    stIsland = parseIsland(argv[1]);
    pstStock = parseStock(argv[2], &nNumProducts);

    if (filterRoutes(&stIsland) < 0 || portInit(&stPort, stIsland.nMaxCapacity) < 0) {
        printF("Error: invalid island configuration\n");
        freeIsland(stIsland);
        free(pstStock);
        exit(EXIT_FAILURE);
    }

    //### stock setup
    if (stockInit(&stStock, pstStock, nNumProducts, argv[2]) < 0) {
        portDestroy(&stPort);
        freeIsland(stIsland);
        free(pstStock);
        exit(EXIT_FAILURE);
    }
    pstStock = NULL;

    //####### 'server' setup
    nServerFd = createServerSocket(stIsland.psIpAddress, stIsland.nPort);
    if (nServerFd < 0) {
        nLen = asprintf(&psErr, "Error: Failed to create server socket on %s:%d (%s)\n",
                 stIsland.psIpAddress, stIsland.nPort, strerror(errno));
        if (nLen >= 0) {
            write(STDOUT_FILENO, psErr, nLen);
            free(psErr);
        }
        portDestroy(&stPort);
        stockDestroy(&stStock);
        freeIsland(stIsland);
        exit(EXIT_FAILURE);
    }

    nLen = asprintf(&psBuf, "Island %s initialized.\nPort capacity: %d %s.\n%d sea routes loaded.\n%d products available.\n\n",
                stIsland.psName,
                stIsland.nMaxCapacity, stIsland.nMaxCapacity == 1 ? "ship" : "ships",
                stIsland.nNumRoutes,
                nNumProducts);
    if (nLen >= 0) {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    //########## main client loop
    while (!gnStop) {
        int nClientFd = acceptClient(nServerFd);
        if (nClientFd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        tIslandSession *pstSession = calloc(1, sizeof(tIslandSession));
        if (pstSession == NULL) {
            close(nClientFd);
            continue;
        }
        pstSession->nFd = nClientFd;
        pstSession->pstPort = &stPort;
        pstSession->psIslandName = stIsland.psName;
        pstSession->eState = SHIP_NONE;
        pstSession->pstStock = &stStock;
        pstSession->pstIsland = &stIsland;

        pthread_t nThreadId;
        if (pthread_create(&nThreadId, NULL, shipThread, pstSession) != 0) {
            close(nClientFd);
            free(pstSession);
        } else {
            pthread_detach(nThreadId);
        }
    }

    // free all island related resources
    portClose(&stPort);
    nLen = asprintf(&psBuf, "\n%s closes its port.\n", stIsland.psName);
    if (nLen >= 0) {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    // removes the semaphore from the kernel
    portDestroy(&stPort);
    closeSocket(&nServerFd);
    stockDestroy(&stStock);
    freeIsland(stIsland);
    return 0;
}