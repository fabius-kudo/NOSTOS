/***********************************************
 *
 * @File : island_main.c
 * @Purpose : Main function for the Island application
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
#include "island_parser.h"
#include "SPHRAGIS Library-20260917/sphragis.h"

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
 * @Def: Main entry point for the island application.
 * @Arg: In: argc number of command line arguments
 *       In: argv array of command line argument strings
 * @Ret: Returns 0 on success, or non-zero status code on error.
 *
 ***********************************************/
int main(int argc, char *argv[]) {
    int nNumProducts = 0;
    int nLen = 0;
    char *psBuf = NULL;
    tProduct *pstStock = NULL;
    tIsland stIsland;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./island <config.dat> <stock.dat>\n", strlen("Usage: ./island <config.dat> <stock.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handleExit);
    
    stIsland = parseIsland(argv[1]);
    pstStock = parseStock(argv[2], &nNumProducts);

    // Validate island configuration using SPHRAGIS library
    if (filterRoutes(&stIsland) < 0) {
        write(STDERR_FILENO, "Error: invalid island configuration\n", strlen("Error: invalid island configuration\n"));
        freeIsland(stIsland);
        free(pstStock);
        exit(EXIT_FAILURE);
    }

    nLen = asprintf(&psBuf, "Island %s initialized.\nPort capacity: %d.\n%d sea routes loaded.\n%d products available.\n", stIsland.psName, stIsland.nMaxCapacity, stIsland.nNumRoutes, nNumProducts);
    if (nLen < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        free(pstStock);
        freeIsland(stIsland);
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    while (!gnStop) {
        pause();
    }

    nLen = asprintf(&psBuf, "\n%s closes its port.\n", stIsland.psName);
    if (nLen < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        free(pstStock);
        freeIsland(stIsland);
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, psBuf, nLen);
        free(psBuf);
    }

    free(pstStock);
    freeIsland(stIsland);

    return 0;
}