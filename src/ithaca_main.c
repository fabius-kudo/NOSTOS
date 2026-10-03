/***********************************************
 *
 * @File : ithaca_main.c
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

// Project Includes
#include "ithaca_parser.h"

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

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./ithaca <config.dat> <voyages.dat>\n", strlen("Usage: ./ithaca <config.dat> <voyages.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handleExit);

    stIthaca = parseIthaca(argv[1]);
    pstVoyages = parseVoyages(argv[2], &nNumVoyages);

    nLen = asprintf(&psBuf, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", nNumVoyages);
    if (nLen < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    while (!gnStop) {
        pause();
    }
    
    write(STDOUT_FILENO, "\nIthaca closes the harbor.\n", strlen("\nIthaca closes the harbor.\n"));

    freeIthaca(&stIthaca);
    freeVoyages(pstVoyages, nNumVoyages);

    return 0;
}