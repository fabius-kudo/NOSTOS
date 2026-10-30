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
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>

// Project Includes
#include "voyage_config.h"
#include "voyage_service.h"
#include "socket_utils.h"

static volatile int gnStop = 0;

/***********************************************
 *
 * @Name: handleExit
 * @Def: Signal handler function for SIGINT interrupt signal.
 * @Arg: In: nSig received signal number
 * @Ret: None.
 *
 ***********************************************/
void handleExit(int nSig) {
    (void)nSig;
    gnStop = 1;
}

static void *clientThread(void *pArg) {
    sigset_t stSet;
    sigemptyset(&stSet);
    sigaddset(&stSet, SIGINT);
    pthread_sigmask(SIG_BLOCK, &stSet, NULL);   // only the main thread handles Ctrl+C

    tClientSession *pstSession = (tClientSession *)pArg;
    serveClient(pstSession);
    closeSocket(&pstSession->nFd);
    free(pstSession);
    return NULL;
}

/***********************************************
 *
 * @Name: main
 * @Def: Main entry point for the Ithaca application.
 * @Arg: In: argc number of command line arguments
 *       In: argv array of command line argument strings
 * @Ret: Returns 0 on success, or non-zero status code on error.
 *
 ***********************************************/
int main(int argc, char *argv[]) {
    int nNumVoyages = 0;
    int nLen = 0;
    char *psBuf = NULL;
    tIthaca stIthaca;
    tVoyage *pstVoyages = NULL;

    // ########## INIT FROM FILE ###########
    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./ithaca <config.dat> <voyages.dat>\n", strlen("Usage: ./ithaca <config.dat> <voyages.dat>\n"));
        exit(EXIT_FAILURE);
    }

    //SIGNAL crtl c
    struct sigaction stAct;
    memset(&stAct, 0, sizeof(stAct));
    stAct.sa_handler = handleExit;
    sigemptyset(&stAct.sa_mask);
    stAct.sa_flags = 0;                  // no SA_RESTART: accept() returns -1 with EINTR
    sigaction(SIGINT, &stAct, NULL);

    stIthaca = parseIthaca(argv[1]);
    pstVoyages = parseVoyages(argv[2], &nNumVoyages);

    tVoyageStore stStore;
    stStore.pstVoyages = pstVoyages;
    stStore.nNumVoyages = nNumVoyages;
    pthread_mutex_init(&stStore.stMutex, NULL);

    //test TODO: remove
    nLen = asprintf(&psBuf, "%s %s %s %d\n\n", stIthaca.psServerName, stIthaca.psPath, stIthaca.psIpAddress, stIthaca.nPort);
    write(STDOUT_FILENO, psBuf, nLen);
    free(psBuf);

    // ############# SERVER INIT #########

    int nServerFd = createServerSocket(stIthaca.psIpAddress, stIthaca.nPort);
    if (nServerFd < 0) {
        char *err;
        int len = asprintf(&err, "Error: Failed to create server socket on %s:%d (%s)\n",
                 stIthaca.psIpAddress, stIthaca.nPort, strerror(errno));
        freeIthaca(&stIthaca);
        freeVoyages(pstVoyages, nNumVoyages);
        exit(EXIT_FAILURE);
    }

    nLen = asprintf(&psBuf, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", nNumVoyages);
    if (nLen < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        closeSocket(&nServerFd);
        freeIthaca(&stIthaca);
        freeVoyages(pstVoyages, nNumVoyages);
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    //  ######### SERVER LOOP (accept clients) #####
    while (!gnStop) {
        int nClientFd = acceptClient(nServerFd);
        if (nClientFd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        tClientSession *pstSession = calloc(1, sizeof(tClientSession));
        if (!pstSession) {
            close(nClientFd);
            continue;
        }
        pstSession->nFd = nClientFd;
        pstSession->pstStore = &stStore;

        pthread_t nThreadId;
        if (pthread_create(&nThreadId, NULL, clientThread, pstSession) != 0) {
            close(nClientFd);
            free(pstSession);
        } else {
            pthread_detach(nThreadId);
        }
    }
    
    write(STDOUT_FILENO, "\nIthaca closes the harbor.\n", strlen("\nIthaca closes the harbor.\n"));

    closeSocket(&nServerFd);
    pthread_mutex_destroy(&stStore.stMutex);
    freeIthaca(&stIthaca);
    freeVoyages(pstVoyages, nNumVoyages);

    return 0;
}