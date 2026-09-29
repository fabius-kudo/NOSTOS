#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "ithaca_parser.h"

static volatile sig_atomic_t stop = 0;

void handle_exit(int sig) {
    (void)sig; //used to avoid unused parameter warning
    stop = 1;
}

int main(int argc, char *argv[]) {
    Ithaca ithaca;
    Voyage *voyages = NULL;
    int num_voyages = 0;
    char buf[100];

    if (argc != 3) {
        //TODO: make real error message
        write(STDERR_FILENO, "Usage: ./ithaca <config.dat> <voyages.dat>\n", 44);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_exit);

    char *config_path = argv[1];
    char *voyages_path = argv[2];

    ithaca = parseIthaca(config_path);
    voyages = parseVoyages(voyages_path, &num_voyages);

    int len = snprintf(buf, sizeof(buf), "Ithaca initialized. %d voyages loaded.\nWaiting for Odysseus...\n", num_voyages);
    write(STDOUT_FILENO, buf, len);

    while (!stop) {
        pause();
    }
    
    write(STDOUT_FILENO, "\nIthaca closes the harbor.\n", strlen("\nIthaca closes the harbor.\n"));

    free(voyages);

    return 0;
}