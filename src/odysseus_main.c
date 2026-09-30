#include "custom_string.h"
#include "odysseus_command.h"
#include "odysseus_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static volatile sig_atomic_t stop = 0;

void handle_exit(int sig) {
    (void)sig; //used to avoid unused parameter warning
    stop = 1;
}

int main(int argc, char *argv[]) {
    char *line;
    
    if (argc != 2) {
        //TODO: add error message
        write(STDOUT_FILENO,"ERROR\n",strlen("ERROR\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_exit);

    char *config_path = argv[1];

    Odysseus odysseus = parseOdysseus(config_path);

    while ((line = readLineDynamic(STDIN_FILENO)) != NULL && !stop) {
        char *argv[MAX_ARGS];
        int argc = tokenizeLine(line, argv, MAX_ARGS);

        processLine(argv, argc);

        free(line);
    }

    write(STDOUT_FILENO, "\nOdysseus stops his journey.\n", strlen("\nOdysseus stops his journey.\n"));

    return 0;
}