/***********************************************
 *
 * @File : ithaca_main.c
 * @Purpose : Main function for the Ithaca application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "ithaca_parser.h"

static volatile int stop = 0;

void handle_exit(int sig) {
    (void)sig; //used to avoid unused parameter warning
    stop = 1;
}

int main(int argc, char *argv[]) {
    Ithaca ithaca;
    Voyage *voyages = NULL;
    int num_voyages = 0;
    char *buf;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./ithaca <config.dat> <voyages.dat>\n", strlen("Usage: ./ithaca <config.dat> <voyages.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_exit);

    ithaca = parseIthaca(argv[1]);
    voyages = parseVoyages(argv[2], &num_voyages);

    int len = asprintf(&buf, "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", num_voyages);
    if (len < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, buf, len);
        free(buf);
    }

    printIthaca(&ithaca);
    printVoyages(voyages, num_voyages);

    while (!stop) {
        pause();
    }
    
    write(STDOUT_FILENO, "\nIthaca closes the harbor.\n", strlen("\nIthaca closes the harbor.\n"));

    freeIthaca(&ithaca);
    freeVoyages(voyages, num_voyages);

    return 0;
}