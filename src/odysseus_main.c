#include "custom_string.h"
#include "odysseus_command.h"
#include "odysseus_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

static volatile int stop = 0;

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
    siginterrupt(SIGINT, 1);    //TODO: make sigaction:

    // struct sigaction sa;
    // sa.sa_handler = handle_exit;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = 0; /* no SA_RESTART: read() returns EINTR immediately instead of resuming */
    // sigaction(SIGINT, &sa, NULL);

    char *config_path = argv[1];

    //Odysseus odysseus = parseOdysseus(config_path);

    while (!stop && (line = readLineDynamic(STDIN_FILENO)) != NULL) {
        char *argv[MAX_ARGS];
        int argc = tokenizeLine(line, argv, MAX_ARGS);

        processLine(argv, argc);

        free(line);
    }

    write(STDOUT_FILENO, "\nOdysseus stops his journey.\n", strlen("\nOdysseus stops his journey.\n"));

    return 0;
}