/***********************************************
 *
 * @File : odysseus.c
 * @Purpose : Main function for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

// Project Includes
#include "custom_dynamic.h"
#include "commands.h"
#include "ship_config.h"

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
    // Stops extra reads from stdin
    close(STDIN_FILENO);
    gnStop = 1;
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
    char *psLine = NULL;
    char *apsTokens[MAX_ARGS];
    //tOdysseus stOdysseus;
    tShip stShip;
    
    if (argc != 2) {
        write(STDOUT_FILENO,"Usage: ./odysseus <config.dat>\n", strlen("Usage: ./odysseus <config.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handleExit);

    //init ship

    memset(&stShip, 0, sizeof(stShip)); 
    stShip.nIthacaFd = -1;
    stShip.nIslandFd = -1;

    stShip.stConfig = parseOdysseus(argv[1]);


    while (!gnStop) {
        write(STDOUT_FILENO, "$ ", 2);
        psLine = readDynamic(STDIN_FILENO, 1);
        if (psLine == NULL) break;
        nTokenCount = tokenizeLine(psLine, apsTokens, MAX_ARGS);
        processLine(&stShip, apsTokens, nTokenCount);
        free(psLine);
    }

    

    write(STDOUT_FILENO, "\nOdysseus stops his journey.\n", 29);

    releaseShip(&stShip);
    if (stShip.nIthacaFd >= 0) close(stShip.nIthacaFd);
    if (stShip.nIslandFd >= 0) close(stShip.nIslandFd);
    free(stShip.psLocation);

    if (gnStop) {
        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
    return 0;
}