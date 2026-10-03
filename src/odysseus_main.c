/***********************************************
 *
 * @File : odysseus_main.c
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
#include "odysseus_command.h"
#include "odysseus_parser.h"

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
    tOdysseus stOdysseus;
    
    if (argc != 2) {
        write(STDOUT_FILENO,"Usage: ./odysseus <config.dat>\n", strlen("Usage: ./odysseus <config.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handleExit);

    stOdysseus = parseOdysseus(argv[1]);


    while (!gnStop && (psLine = readDynamic(STDIN_FILENO, 1)) != NULL) {
        nTokenCount = tokenizeLine(psLine, apsTokens, MAX_ARGS);
        processLine(apsTokens, nTokenCount);
        free(psLine);
    }

    write(STDOUT_FILENO, "\nOdysseus stops his journey.\n", 29);
    freeOdysseus(&stOdysseus);

    if (gnStop) {
        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
    return 0;
}