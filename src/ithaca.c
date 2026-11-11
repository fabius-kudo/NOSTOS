/***********************************************
 *
 * @File : ithaca.c
 * @Purpose : Main function for the Ithaca application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

// System Includes
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

// Project Includes
#include "voyage_config.h"
#include "voyage_service.h"
#include "socket_utils.h"
#include "client_list.h"

int gnServerFd = -1;
pthread_t gtMainThread;

/***********************************************
 *
 * @Name: handleExit
 * @Def: CTRL+C handler. Closes the listening socket, which makes the accept
 *       of the main loop fail and starts the shutdown of Ithaca.
 * @Arg: In: nSig received signal number
 * @Ret: None.
 *
 ***********************************************/
void handleExit(int nSig) {
    (void)nSig;

    // The signal can land in any thread: it is always passed on to main
    if (!pthread_equal(pthread_self(), gtMainThread)) {
        pthread_kill(gtMainThread, SIGINT);
        return;
    }
    closeSocket(&gnServerFd);
}

/***********************************************
 *
 * @Name: clientThread
 * @Def: Thread of one client: attends it until it disconnects.
 * @Arg: In/Out: pArg node of this thread in the client list
 * @Ret: NULL.
 *
 ***********************************************/
void *clientThread(void *pArg) {
    tClientNode *pstNode = (tClientNode *)pArg;
    tClientSession *pstSession = (tClientSession *)pstNode->pvData;

    serveClient(pstSession);

    // A client that vanished without disconnecting still has its name
    free(pstSession->psName);
    pstSession->psName = NULL;

    //Main thread frees after joining
    clientListMarkDone(pstNode);
    return NULL;
}


int main(int argc, char *argv[]) {
    int nNumVoyages = 0;
    int nLen = 0;
    int nClientFd = -1;
    char *psBuf = NULL;
    tIthaca stIthaca;
    tVoyage *pstVoyages = NULL;
    tVoyageStore stStore;
    tClientSession *pstSession = NULL;
    tClientNode *pstNode = NULL;
    tClientList stClients;

    // ########## INIT FROM FILE ###########
    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./ithaca <config.dat> <voyages.dat>\n", strlen("Usage: ./ithaca <config.dat> <voyages.dat>\n"));
        exit(EXIT_FAILURE);
    }

    //SIGNAL crtl c
    gtMainThread = pthread_self();
    signal(SIGINT, handleExit);
    signal(SIGPIPE, SIG_IGN);

    stIthaca = parseIthaca(argv[1]);
    pstVoyages = parseVoyages(argv[2], &nNumVoyages);

    stStore.pstVoyages = pstVoyages;
    stStore.nNumVoyages = nNumVoyages;
    pthread_mutex_init(&stStore.stMutex, NULL);

    // ############# SERVER INIT #########

    gnServerFd = createServerSocket(stIthaca.psIpAddress, stIthaca.nPort);
    if (gnServerFd < 0) {
        nLen = asprintf(&psBuf, "Error: Failed to create server socket on %s:%d\n",
                        stIthaca.psIpAddress, stIthaca.nPort);
        if (nLen >= 0) {
            write(STDOUT_FILENO, psBuf, nLen);
            free(psBuf);
        }
        pthread_mutex_destroy(&stStore.stMutex);
        freeIthaca(&stIthaca);
        freeVoyages(pstVoyages, nNumVoyages);
        exit(EXIT_FAILURE);
    }

    nLen = asprintf(&psBuf, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", nNumVoyages);
    if (nLen < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        closeSocket(&gnServerFd);
        pthread_mutex_destroy(&stStore.stMutex);
        freeIthaca(&stIthaca);
        freeVoyages(pstVoyages, nNumVoyages);
        exit(EXIT_FAILURE);
    }
    write(STDOUT_FILENO, psBuf, nLen);
    free(psBuf);

    clientListInit(&stClients);

    //  ######### SERVER LOOP 
    while (gnServerFd >= 0) {
        nClientFd = acceptClient(gnServerFd);
        if (nClientFd < 0) {
            break;
        }

        // Join the clients that already finished, so the list does not keep growing
        clientListReap(&stClients, 0);

        pstSession = malloc(sizeof(tClientSession));
        if (pstSession == NULL) {
            close(nClientFd);
            continue;
        }
        memset(pstSession, 0, sizeof(tClientSession));
        pstSession->nFd = nClientFd;
        pstSession->pstStore = &stStore;

        // From here the list owns the session: it is freed with the node
        pstNode = clientListAdd(&stClients, nClientFd, pstSession);
        if (pstNode == NULL) {
            free(pstSession);
            close(nClientFd);
            continue;
        }
        if (pthread_create(&pstNode->tThreadId, NULL, clientThread, pstNode) != 0) {
            clientListDrop(&stClients, pstNode);
            close(nClientFd);
        }
    }

    write(STDOUT_FILENO, "\nIthaca closes the harbor.\n", strlen("\nIthaca closes the harbor.\n"));

    // 1. no new clients (does nothing if the handler already closed it)
    closeSocket(&gnServerFd);

    // 2. every client thread is blocked reading its socket: shutting it down makes that read end, so each thread leaves through its normal exit
    clientListWake(&stClients);

    // 3. wait for every client thread; their sockets and sessions are freed here
    clientListReap(&stClients, 1);

    // 4. no thread is left: the shared data can be destroyed safely
    clientListDestroy(&stClients);
    pthread_mutex_destroy(&stStore.stMutex);
    freeIthaca(&stIthaca);
    freeVoyages(pstVoyages, nNumVoyages);

    return 0;
}