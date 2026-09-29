#include "ithaca_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

Ithaca parseIthaca(char *config_path) {
    Ithaca ithaca;
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
    strncpy(ithaca.serverName, line, sizeof(ithaca.serverName) - 1);
    ithaca.serverName[sizeof(ithaca.serverName) - 1] = '\0';

    // Line 2: path
    line = strtok(NULL, "\n");
    strncpy(ithaca.path, line, sizeof(ithaca.path) - 1);
    ithaca.path[sizeof(ithaca.path) - 1] = '\0';

    // Line 3: IP + port
    line = strtok(NULL, "\n");
    char *tok = strtok(line, " ");
    strncpy(ithaca.ip_address, tok, sizeof(ithaca.ip_address) - 1);
    ithaca.ip_address[sizeof(ithaca.ip_address) - 1] = '\0';
    tok = strtok(NULL, " ");
    ithaca.port = atoi(tok);

    return ithaca;
}

Voyage *parseVoyages(char *voyages_path, int *num_voyages) {
    int fd = open(voyages_path, O_RDONLY);
    if (fd < 0) {
        //TODO: make real error message
        exit(EXIT_FAILURE);
    }

    // Read whole file into buffer
    char buf[4000]; //TODO: make dynamic
    int total = 0, n;
    while ((n = read(fd, &buf[total], sizeof(buf) - total - 1)) > 0) {
        total += n;
    }
    close(fd);

    if (n < 0) {
        write(STDERR_FILENO, "Error: failed reading voyages file\n", 36);
        exit(EXIT_FAILURE);
    }
    buf[total] = '\0';

    // First pass: count lines
    char buf_copy[4000];
    strncpy(buf_copy, buf, sizeof(buf_copy) - 1);
    buf_copy[sizeof(buf_copy) - 1] = '\0';

    int count = 0;
    char *line = strtok(buf_copy, "\n");
    while (line != NULL) {
        count++;
        line = strtok(NULL, "\n");
    }

    Voyage *voyages = malloc(sizeof(Voyage) * count);
    if (!voyages && count > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", 21);
        exit(EXIT_FAILURE);
    }
    
    // Second pass: parse each line into a Voyage
    line = strtok(buf, "\n");
    int i = 0;
    while (line != NULL) {
        char *tok = strtok(line, " ");
        strncpy(voyages[i].object, tok, sizeof(voyages[i].object) - 1);
        voyages[i].object[sizeof(voyages[i].object) - 1] = '\0';

        tok = strtok(NULL, " ");
        strncpy(voyages[i].file, tok, sizeof(voyages[i].file) - 1);
        voyages[i].file[sizeof(voyages[i].file) - 1] = '\0';

        tok = strtok(NULL, " ");
        strncpy(voyages[i].destination, tok, sizeof(voyages[i].destination) - 1);
        voyages[i].destination[sizeof(voyages[i].destination) - 1] = '\0';

        tok = strtok(NULL, " ");
        voyages[i].reward = atoi(tok);

        line = strtok(NULL, "\n");
        i++;
    }

    *num_voyages = count;
    return voyages;
}