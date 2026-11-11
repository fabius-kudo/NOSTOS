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
#include <signal.h>
#include <pthread.h>

#include "port_config.h"
#include "port_control.h"
#include "island_service.h"
#include "socket_utils.h"
#include "island_stock.h"
#include "custom_io.h"
#include "client_list.h"

//global thread and socket for sig handling
int gnServerFd = -1;
pthread_t gtMainThread;

void handleExit(int nSig) {
    (void)nSig;
 
    // The signal can land in any thread: it is always passed on to main
    if (!pthread_equal(pthread_self(), gtMainThread)) {
        pthread_kill(gtMainThread, SIGINT);
        return;
    }
    closeSocket(&gnServerFd);
}


void *shipThread(void *pArg) {
    tClientNode *pstNode = (tClientNode *)pArg;
 
    serveShip((tIslandSession *)pstNode->pvData);
 
    // Last action. The socket and the session are not released here: the
    // main thread does it after joining this thread
    clientListMarkDone(pstNode);
    return NULL;
}

int main(int argc, char *argv[]) {
    int nNumProducts = 0;
    int nLen = 0;
    int nClientFd = -1;
    char *psBuf = NULL;
    tProduct *pstStock = NULL;
    tIslandSession *pstSession = NULL;
    tClientNode *pstNode = NULL;
    tIsland stIsland;
    tPort stPort;
    tStock stStock;
    tClientList stClients;
 
    if (argc != 3) {
        printF("Usage: ./island <config.dat> <stock.db>\n");
        exit(EXIT_FAILURE);
    }
 
    gtMainThread = pthread_self();
    signal(SIGINT, handleExit);
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
    gnServerFd = createServerSocket(stIsland.psIpAddress, stIsland.nPort);
    if (gnServerFd < 0) {
        nLen = asprintf(&psBuf, "Error: Failed to create server socket on %s:%d\n",
                        stIsland.psIpAddress, stIsland.nPort);
        if (nLen >= 0) {
            write(STDOUT_FILENO, psBuf, nLen);
            free(psBuf);
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
 
    clientListInit(&stClients);
 
    //########## main client loop: ends when CTRL+C closes the listening socket
    while (gnServerFd >= 0) {
        nClientFd = acceptClient(gnServerFd);
        if (nClientFd < 0) {
            break;
        }
 
        // Join the ships that already finished, so the list does not keep growing
        clientListReap(&stClients, 0);
 
        pstSession = malloc(sizeof(tIslandSession));
        if (pstSession == NULL) {
            close(nClientFd);
            continue;
        }
        memset(pstSession, 0, sizeof(tIslandSession));
        pstSession->nFd = nClientFd;
        pstSession->pstPort = &stPort;
        pstSession->psIslandName = stIsland.psName;
        pstSession->eState = SHIP_NONE;
        pstSession->pstStock = &stStock;
        pstSession->pstIsland = &stIsland;
 
        // From here the list owns the session: it is freed with the node
        pstNode = clientListAdd(&stClients, nClientFd, pstSession);
        if (pstNode == NULL) {
            free(pstSession);
            close(nClientFd);
            continue;
        }
        if (pthread_create(&pstNode->tThreadId, NULL, shipThread, pstNode) != 0) {
            clientListDrop(&stClients, pstNode);
            close(nClientFd);
        }
    }
 
    // 1. no new ships (does nothing if the handler already closed it)
    closeSocket(&gnServerFd);
 
    // 2. the queued ships wake up, see the port is closing and end
    portClose(&stPort);
    nLen = asprintf(&psBuf, "\n%s closes its port.\n", stIsland.psName);
    if (nLen >= 0) {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }
 
    // 3. the docked ships are blocked reading their socket: shutting it down
    //    makes that read end, so each thread leaves through its normal exit
    clientListWake(&stClients);
 
    // 4. wait for every ship thread; their sockets and sessions are freed here
    clientListReap(&stClients, 1);
 
    // 5. no thread is left: the shared data can be destroyed safely
    clientListDestroy(&stClients);
    portDestroy(&stPort);                       // removes the semaphore from the kernel
    stockDestroy(&stStock);
    freeIsland(stIsland);
    return 0;
}
 