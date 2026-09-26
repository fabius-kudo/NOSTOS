#include "Ithaca.h"

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
    strncpy(ithaca.ipAddress, tok, sizeof(ithaca.ipAddress) - 1);
    ithaca.ipAddress[sizeof(ithaca.ipAddress) - 1] = '\0';
    tok = strtok(NULL, " ");
    ithaca.port = atoi(tok);

    return ithaca;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        //TODO: make real error message
        write(STDERR_FILENO, "Usage: ./ithaca <config.dat> <voyages.dat>\n", 44);
        exit(EXIT_FAILURE);
    }

    char *config_path = argv[1];
    char *voyages_path = argv[2];

    Ithaca ithaca = parseIthaca(config_path);
}