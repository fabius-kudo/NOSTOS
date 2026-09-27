#include "Island.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

Island parseIsland(char *islands_path) {
    Island island;

    int fd = open(islands_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed to open island file\n", 36);
        exit(EXIT_FAILURE);
    }

    //Read whole file into buffer
    char buf[1000]; //TODO: make dynamic
    int total = 0, n;
    while ((n = read(fd, &buf[total], sizeof(buf) - total - 1)) > 0) {
        total += n;
    }
    close(fd);

    if (n < 0) {
        write(STDERR_FILENO, "Error: failed reading config file\n", 35);
        exit(EXIT_FAILURE);
    }
    buf[total] = '\0';

    // Line 1: name
    char *line = strtok(buf, "\n");
    strncpy(island.name, line, sizeof(island.name) - 1);
    island.name[sizeof(island.name) - 1] = '\0';

    // Line 2: path
    line = strtok(NULL, "\n");
    strncpy(island.path, line, sizeof(island.path) - 1);
    island.path[sizeof(island.path) - 1] = '\0';

    // Line 3: IP + port
    line = strtok(NULL, "\n");
    char *tok = strtok(line, " ");
    strncpy(island.ipAddress, tok, sizeof(island.ipAddress) - 1);
    island.ipAddress[sizeof(island.ipAddress) - 1] = '\0';
    tok = strtok(NULL, " ");
    island.port = atoi(tok);

    // Line 4: max capacity
    line = strtok(NULL, "\n");
    island.maxCapacity = atoi(line);

    // Line 5: comment
    line = strtok(NULL, "\n");

    // Line 6: routes
    int numRoutes = 0;
    for (int i = 0; i < MAX_ROUTES; i++) {
        line = strtok(NULL, "\n");
        if (line == NULL) {
            break;
        }
        tok = strtok(line, " ");
        strncpy(island.routes[i].name, tok, sizeof(island.routes[i].name) - 1);
        island.routes[i].name[sizeof(island.routes[i].name) - 1] = '\0';

        tok = strtok(NULL, " ");
        strncpy(island.routes[i].ipAddress, tok, sizeof(island.routes[i].ipAddress) - 1);
        island.routes[i].ipAddress[sizeof(island.routes[i].ipAddress) - 1] = '\0';

        tok = strtok(NULL, " ");
        island.routes[i].port = atoi(tok);

        numRoutes++;
    }
    island.numRoutes = numRoutes;

    return island;
}

Product *parseStock(char *stock_path, int *num_products) {
    Product *products = NULL;
    int count = 0;

    int fd = open(stock_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed to open stock file\n", 34);
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

int main(int argc, char *argv[]) {
    Island island;
    Product *stock = NULL;
    int num_products = 0;

    if (argc != 3) {
        //TODO: make real error message
        write(STDERR_FILENO, "Error: invalid number of arguments\n", 37);
        exit(EXIT_FAILURE);
    }

    char *config_path = argv[1];
    char *stock_path = argv[2];

    island = parseIsland(config_path);
    stock = parseStock(stock_path, &num_products);

    free(stock);
    
    return 0;
}