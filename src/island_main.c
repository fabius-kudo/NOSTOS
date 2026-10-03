#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "island_parser.h"
#include "SPHRAGIS Library-20260917/sphragis.h"

static volatile int stop = 0;

void handle_exit(int sig) {
    (void)sig; //used to avoid unused parameter warning
    stop = 1;
}

int main(int argc, char *argv[]) {
    Island island;
    Product *stock = NULL;
    int num_products = 0;
    char *buf;

    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./island <config.dat> <stock.dat>\n", strlen("Usage: ./island <config.dat> <stock.dat>\n"));
        free(buf);
        free(stock);
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_exit);
    
    island = parseIsland(argv[1]);
    stock = parseStock(argv[2], &num_products);

    printIsland(&island);

    if (filterRoutes(&island) < 0) {
        write(STDERR_FILENO, "Error: invalid island configuration\n", strlen("Error: invalid island configuration\n"));
        freeIsland(island);
        free(stock);
        exit(EXIT_FAILURE);}

    printIsland(&island);
    printProducts(stock, num_products);

    while (!stop) {
        pause();
    }

    int len = asprintf(&buf, "\n%s closes its port.\n", island.name);
    if (len < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, buf, len);
        free(buf);
    }

    free(stock);
    freeIsland(island);

    return 0;
}