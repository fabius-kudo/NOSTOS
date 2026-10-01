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
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_exit);
    
    island = parseIsland(argv[1]);
    stock = parseStock(argv[2], &num_products);

    //TODO add this
    // int new_num_routes = SPHRAGIS_filter_island_configuration(&island);

    // if (new_num_routes == SPHRAGIS_ERROR_INVALID_ISLAND) {
    //     write(STDERR_FILENO, "Invalid island route configuration.\n", strlen("Invalid island route configuration.\n"));
    // } else if (new_num_routes == SPHRAGIS_ERROR_INVALID_CONNECTION)  {
    //     write(STDERR_FILENO, "Invalid connection in island configuration.\n", strlen("Invalid connection in island configuration.\n"));
    // } else {
    //     island.num_routes = new_num_routes;
    // }
    
    int len = asprintf(&buf, "Island %s initialized.\nPort capacity: %d.\n%d sea routes loaded.\n%d products available.\n", island.name, island.max_capacity, island.num_routes, num_products);
    if (len < 0) {
        write(STDERR_FILENO, "Error: asprintf failed\n", strlen("Error: asprintf failed\n"));
        exit(EXIT_FAILURE);
    } else {
        write(STDOUT_FILENO, buf, len);
    }

    printIsland(&island);

    while (!stop) {
        pause();
    }

    len = asprintf(&buf, "\n%s closes its port.\n", island.name);
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