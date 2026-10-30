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
    tProduct *pstStock = NULL;
    tIsland stIsland;
    tPort stPort;
    struct sigaction stAct;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./island <config.dat> <stock.dat>\n", strlen("Usage: ./island <config.dat> <stock.dat>\n"));
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
        write(STDERR_FILENO, "Error: invalid island configuration\n", strlen("Error: invalid island configuration\n"));
        freeIsland(stIsland);
        free(pstStock);
        exit(EXIT_FAILURE);
    }

    //####### 'server' setup
    nServerFd = createServerSocket(stIsland.psIpAddress, stIsland.nPort);
    if (nServerFd < 0) {
        char *err;
        int len = asprintf(&err, "Error: Failed to create server socket on %s:%d (%s)\n",
                 stIsland.psIpAddress, stIsland.nPort, strerror(errno));
        write(STDOUT_FILENO, err, len);
        portDestroy(&stPort);
        freeIsland(stIsland);
        free(pstStock);
        exit(EXIT_FAILURE);
    }

    nLen = asprintf(&psBuf, "%s is open. Docks: %d.\nWaiting for ships...\n", stIsland.psName, stIsland.nMaxCapacity);
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

        pthread_t nThreadId;
        if (pthread_create(&nThreadId, NULL, shipThread, pstSession) != 0) {
            close(nClientFd);
            free(pstSession);
        } else {
            pthread_detach(nThreadId);
        }
    }

    portClose(&stPort);  // wakes queued ships
    nLen = asprintf(&psBuf, "\n%s closes its port.\n", stIsland.psName);
    if (nLen >= 0) {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    // TODO step 8: shutdown() the active client fds and join the ship threads before
    // destroying the port, so no thread is still using the mutex or the semaphore.
    portDestroy(&stPort);                       // removes the System V semaphore from the kernel
    closeSocket(&nServerFd);
    free(pstStock);
    freeIsland(stIsland);
    return 0;
}