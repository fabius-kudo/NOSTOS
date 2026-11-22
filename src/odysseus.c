/***********************************************
 *
 * @File : odysseus.c
 * @Purpose : Main function for the Odysseus application
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

// Project Includes
#include "custom_io.h"
#include "commands.h"
#include "ship_config.h"
#include "ship_state.h"

int gnStop = 0;

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
    // Stops extra reads from stdin
    close(STDIN_FILENO);
    gnStop = 1;
}

/***********************************************
 *
 * @Name: reportRestore
 * @Def: Tells the player whether the progress of a previous execution was
 *       restored, and how to get back into port.
 * @Arg: In: pstShip ship state after loading
 *       In: nRes result of loadShipState
 * @Ret: None.
 *
 ***********************************************/
void reportRestore(const tShip *pstShip, int nRes) {
    char *psMsg = NULL;

    if (nRes == STATE_DAMAGED) {
        printF("Warning: the saved progress could not be read. Starting from the configuration.\n");
        return;
    }
    if (nRes == STATE_NONE) {
        return;
    }

    printF("Previous journey restored.\n");
    if (nRes == STATE_VOYAGE_LOST) {
        printF("The object of the voyage is no longer on board: the voyage has been lost.\n");
    }
    if (pstShip->psLocation == NULL) {
        return;
    }
    // The ship is not connected anywhere until it reconnects
    if (strcasecmp(pstShip->psLocation, "Ithaca") == 0) {
        if (asprintf(&psMsg, "Current location: Ithaca (not connected). Use CONNECT ITHACA to reconnect.\n") < 0) {
            psMsg = NULL;
        }
    } else if (asprintf(&psMsg, "Current location: %s (not docked). Use SAIL %s to enter its port again.\n",
                        pstShip->psLocation, pstShip->psLocation) < 0) {
        psMsg = NULL;
    }
    printAndFree(psMsg);
}

/***********************************************
 *
 * @Name: main
 * @Def: Main entry point for the Odysseus application.
 * @Arg: In: argc number of command line arguments
 *       In: argv array of command line argument strings
 * @Ret: Returns 0 on success, or non-zero status code on error.
 *
 ***********************************************/
int main(int argc, char *argv[]) {
    int nTokenCount = 0;
    int nRestore;
    char *psLine = NULL;
    char *apsTokens[MAX_ARGS];
    tShip stShip;
    
    if (argc != 2) {
        write(STDOUT_FILENO,"Usage: ./odysseus <config.dat>\n", strlen("Usage: ./odysseus <config.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handleExit);

    //init ship

    stShip.stConfig = parseOdysseus(argv[1]);

    if (initShip(&stShip, stShip.stConfig) < 0) {
        releaseShip(&stShip);
        write(STDERR_FILENO, "Error: out of memory\n", strlen("Error: out of memory\n"));
        return EXIT_FAILURE;
    }
    // Set after initShip, which clears the whole ship
    stShip.pnStop = &gnStop;

    // Continue from a previous execution, if its progress was saved
    nRestore = loadShipState(&stShip);

    char *psMsg = NULL;
    if (asprintf(&psMsg, "Odysseus %s is ready to sail.\n", stShip.stConfig.psName) >= 0) {
        write(STDOUT_FILENO, psMsg, strlen(psMsg));
        free(psMsg);
    }
    reportRestore(&stShip, nRestore);

    while (!gnStop && !stShip.nShouldExit) {
        write(STDOUT_FILENO, "\n$ ", strlen("\n$ "));
        psLine = readDynamic(STDIN_FILENO, 1);
        if (psLine == NULL) break;
        nTokenCount = tokenizeLine(psLine, apsTokens, MAX_ARGS);
        processLine(&stShip, apsTokens, nTokenCount);
        free(psLine);
    }

    write(STDOUT_FILENO, "\nOdysseus stops his journey.\n", strlen("\nOdysseus stops his journey.\n"));

    releaseShip(&stShip);
    if (stShip.nIthacaFd >= 0) close(stShip.nIthacaFd);
    if (stShip.nIslandFd >= 0) close(stShip.nIslandFd);
    free(stShip.psLocation);

    if (gnStop) {
        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
    releaseShip(&stShip);
    return 0;
}