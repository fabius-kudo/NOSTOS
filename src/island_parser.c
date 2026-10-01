#include "island_parser.h"
#include "custom_dynamic.h"
#include "SPHRAGIS Library-20260917/sphragis.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

Island parseIsland(char *islands_path) {
    Island island;
    memset(&island, 0, sizeof(island)); //like malloc and all pointers NULL

    int fd = open(islands_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed to open island file\n", strlen("Error: failed to open island file\n"));
        exit(EXIT_FAILURE);
    }

    //Read whole file into buffer
    char *buf = readDynamic(fd, 0);
    close(fd);
    if (!buf) {
        write(STDERR_FILENO, "Error: malloc failed opening config file\n", strlen("Error: malloc failed opening config file\n"));
        exit(EXIT_FAILURE);
    }

    char *bufCopy = dupString(buf);
    if (!bufCopy) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }
    int count = 0;
    char *line = strtok(bufCopy, "\n");
    while (!strcmp(line, "--- ROUTES ---")) {
        line = strtok(NULL, "\n");
    }
    while (line != NULL) {
        count++;
        line = strtok(NULL, "\n");
    }
    free(bufCopy);

    island.name = dupString(strtok(buf, "\n"));
    island.path = dupString(strtok(NULL, "\n"));
    island.ip_address = dupString(strtok(NULL, " \n"));
    island.port = atoi(strtok(NULL, " \n"));
    island.max_capacity = atoi(strtok(NULL, " \n"));
    island.routes = malloc(sizeof(Route) * count);
    if (!island.routes && count > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < count; i++) {
        island.routes[i].name = dupString(strtok(NULL, " \n"));
        island.routes[i].ip_address = dupString(strtok(NULL, " \n"));
        island.routes[i].port = atoi(strtok(NULL, " \n"));
    }

    island.num_routes = count;

    return island;
}

void freeIsland(Island island) {
    free(island.name);
    free(island.path);
    free(island.ip_address);
    for (int i = 0; i < island.num_routes; i++) {
        free(island.routes[i].name);
        free(island.routes[i].ip_address);
    }
    free(island.routes);
}

Product *parseStock(char *stock_path, int *num_products) {
    Product *products = NULL;
    int count = 0;

    int fd = open(stock_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed to open stock file\n", strlen("Error: failed to open stock file\n"));
        exit(EXIT_FAILURE);
    }

    Product temp;

    while ((read(fd, &temp, sizeof(Product))) == sizeof(Product)) {
        products = realloc(products, sizeof(Product) * (count + 1));
        products[count] = temp;
        count++;
    }

    close(fd);
    *num_products = count;
    return products;
}

//TEST FUNCTION, ILLEGAL TODO:REMOVE
void printIsland(const Island *island) {
    printf("Island Name: %s\n", island->name);
    printf("Path: %s\n", island->path);
    printf("IP Address: %s\n", island->ip_address);
    printf("Port: %d\n", island->port);
    printf("Max Capacity: %d\n", island->max_capacity);
    printf("Number of Routes: %d\n", island->num_routes);
    for (int i = 0; i < island->num_routes; i++) {
        printf("Route %d:\n", i + 1);
        printf("  Name: %s\n", island->routes[i].name);
        printf("  IP Address: %s\n", island->routes[i].ip_address);
        printf("  Port: %d\n", island->routes[i].port);
    }
}