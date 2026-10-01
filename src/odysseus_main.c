#include "custom_dynamic.h"
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
    (void)sig;              // unused parameter
    close(STDIN_FILENO);    //stops extra reads from stdin
    stop = 1;
}
int main(int argc, char *argv[]) {
    char *line;
    
    if (argc != 2) {
        write(STDOUT_FILENO,"Usage: ./odysseus <config.dat>\n", strlen("Usage: ./odysseus <config.dat>\n"));
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_exit);

    Odysseus odysseus = parseOdysseus(argv[1]);
    printOdysseus(&odysseus);   //ILLEGAL TEST FUNCTION TODO: REMOVE


    while (!stop && (line = readDynamic(STDIN_FILENO, 1)) != NULL) {
        char *argv[MAX_ARGS];
        int argc = tokenizeLine(line, argv, MAX_ARGS);

        processLine(argv, argc);
        free(line);
    }

    write(STDOUT_FILENO, "\nOdysseus stops his journey.\n", 29);
    freeOdysseus(&odysseus);

    if (stop) {
        signal(SIGINT, SIG_DFL);
        raise(SIGINT);
    }
    return 0;
}