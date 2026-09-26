#include "Odysseus.h"

Odysseus parseOdysseus(char * config_path) {
    Odysseus od;
    int fd = open(config_path, O_RDONLY);
    if (fd < 0) {
        //TODO: add error message
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
    strncpy(od.name, line, sizeof(od.name) - 1);
    od.name[sizeof(od.name) - 1] = '\0';

    // Line 2: storage path
    line = strtok(NULL, "\n");
    strncpy(od.file_path, line, sizeof(od.file_path) - 1);
    od.file_path[sizeof(od.file_path) - 1] = '\0';

    // Line 3: Ithaca IP + port
    line = strtok(NULL, "\n");
    char *tok = strtok(line, " ");
    strncpy(od.ithaca_ip, tok, sizeof(od.ithaca_ip) - 1);
    od.ithaca_ip[sizeof(od.ithaca_ip) - 1] = '\0';

    tok = strtok(NULL, " ");
    od.ithaca_port = atoi(tok);

    // Line 4: start island -> NAME IP PORT
    line = strtok(NULL, "\n");
    tok = strtok(line, " ");
    strncpy(od.island_name, tok, sizeof(od.island_name) - 1);
    od.island_name[sizeof(od.island_name) - 1] = '\0';

    tok = strtok(NULL, " ");
    strncpy(od.island_ip, tok, sizeof(od.island_ip) - 1);
    od.island_ip[sizeof(od.island_ip) - 1] = '\0';

    tok = strtok(NULL, " ");
    od.islandPort = atoi(tok);

    // Line 5: money
    line = strtok(NULL, "\n");
    od.initial_money = atoi(line);

    // Line 6: number of food items
    line = strtok(NULL, "\n");
    od.num_food_items = atoi(line);

    // Remaining lines: PRODUCT AMOUNT
    od.food_supplies = malloc(sizeof(Food) * od.num_food_items);
    if (!od.food_supplies && od.num_food_items > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", 21);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < od.num_food_items; i++) {
        line = strtok(NULL, "\n");
        tok = strtok(line, " ");
        strncpy(od.food_supplies[i].product, tok, sizeof(od.food_supplies[i].product) - 1);
        od.food_supplies[i].product[sizeof(od.food_supplies[i].product) - 1] = '\0';
        tok = strtok(NULL, " ");
        od.food_supplies[i].quantity = atoi(tok);
    }

    return od;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        //TODO: add error message
        exit(EXIT_FAILURE);
    }

    char *config_path = argv[1];

    Odysseus odysseus = parseOdysseus(config_path);
}