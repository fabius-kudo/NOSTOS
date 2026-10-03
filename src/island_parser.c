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
    memset(&island, 0, sizeof(island));

    int fd = open(islands_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed to open island file\n", strlen("Error: failed to open island file\n"));
        exit(EXIT_FAILURE);
    }

    char *buf = readDynamic(fd, 0);
    close(fd);
    if (!buf) {
        write(STDERR_FILENO, "Error: failed reading island file\n", strlen("Error: failed reading island file\n"));
        exit(EXIT_FAILURE);
    }

    char *bufCopy = dupString(buf);
    if (!bufCopy) {
        free(buf);
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    // Counting lines after marker
    int count = 0;
    char *line = strtok(bufCopy, "\n");
    while (line != NULL && strcmp(line, "--- ROUTES ---") != 0) {
        line = strtok(NULL, "\n");
    }
    if (line != NULL) {                    
        line = strtok(NULL, "\n");
        while (line != NULL) {
            count++;
            line = strtok(NULL, "\n");
        }
    }
    free(bufCopy);

    // Real pass
    island.name = dupString(strtok(buf, "\n"));
    island.path = dupString(strtok(NULL, "\n"));
    island.ip_address = dupString(strtok(NULL, " \n"));
    island.port = atoi(strtok(NULL, " \n"));
    island.max_capacity = atoi(strtok(NULL, " \n"));
    strtok(NULL, "\n");                  // consume the "--- ROUTES ---" line

    island.routes = malloc(sizeof(Route) * count);
    if (!island.routes && count > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < count; i++) {
        island.routes[i].name       = dupString(strtok(NULL, " \n"));
        island.routes[i].ip_address = dupString(strtok(NULL, " \n"));
        island.routes[i].port       = atoi(strtok(NULL, " \n"));
    }
    island.num_routes = count;

    free(buf);
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
    while (read(fd, &temp, sizeof(Product)) == (ssize_t)sizeof(Product)) {
        Product *tmp = realloc(products, sizeof(Product) * (count + 1));
        if (!tmp) {
            free(products);
            close(fd);
            write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
            exit(EXIT_FAILURE);
        }
        products = tmp;
        products[count++] = temp;
    }

    close(fd);
    *num_products = count;
    return products;
}

int filterRoutes(Island *island) {
    SPHRAGIS_Island tmp;
    tmp.name = island->name;
    tmp.known_island_count = island->num_routes;
    tmp.known_islands = NULL;

    if (island->num_routes > 0) {
        tmp.known_islands = calloc(island->num_routes, sizeof(char *));
        if (!tmp.known_islands) return -1;

        for (int i = 0; i < island->num_routes; i++) {
            tmp.known_islands[i] = dupString(island->routes[i].name);
            if (!tmp.known_islands[i]) {
                for (int j = 0; j < i; j++) free(tmp.known_islands[j]);
                free(tmp.known_islands);
                return -1;
            }
        }
    }

    int n = SPHRAGIS_filter_island_configuration(&tmp);
    if (n < 0) {                                   // invalid island or array/count
        for (int i = 0; i < island->num_routes; i++) free(tmp.known_islands[i]);
        free(tmp.known_islands);
        return -1;
    }

    // Survivors keep their order, so a single index into them is enough
    int k = 0;
    for (int i = 0; i < island->num_routes; i++) {
        if (k < n && strcmp(tmp.known_islands[k], island->routes[i].name) == 0) {
            island->routes[k++] = island->routes[i];   // compact in place
        } else {
            free(island->routes[i].name);              // rejected route
            free(island->routes[i].ip_address);
        }
    }
    island->num_routes = n;

    for (int i = 0; i < n; i++) free(tmp.known_islands[i]);   // our remaining copies
    free(tmp.known_islands);
    return n;
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
        printf("  Port: %d\n\n\n", island->routes[i].port);
    }
}

void printProducts(const Product *products, int num_products) {
    for (int i = 0; i < num_products; i++) {
        printf("Product %d:\n", i + 1);
        printf("  Name: %s\n", products[i].name);
        printf("  Quantity: %d\n", products[i].quantity);
        printf("  Price: %d\n", products[i].price);
    }
}