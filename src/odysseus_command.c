#include "odysseus_command.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*  --- Terminal commands ---   */

// --- command functions
static int cmd_def(int argc, char *argv[]){
    
    return 0;
}

static Command commands[] = {
    { "CONNECT", "ITHACA", 0, { 0 }, "Usage: CONNECT ITHACA\n", cmd_def },
    { "LIST", "VOYAGES", 0, { 0 }, "Usage: LIST VOYAGES\n", cmd_def },
    { "ACCEPT", NULL, 1, { ARG_NUMERIC }, "Usage: ACCEPT <voyage_id>\n", cmd_def },
    { "SAIL", NULL, 1, { ARG_TEXT }, "Usage: SAIL <island>\n", cmd_def },
    { "MAP", NULL, 0, { 0 }, "Usage: MAP\n", cmd_def },
    { "LIST", "MARKET", 0, { 0 }, "Usage: LIST MARKET\n", cmd_def },
    { "BUY", NULL, 2, { ARG_TEXT, ARG_NUMERIC }, "Usage: BUY <product> <amount>\n", cmd_def },
    { "SELL", NULL, 2, { ARG_TEXT, ARG_NUMERIC  }, "Usage: SELL <product> <amount>\n", cmd_def },
    { "STATUS", NULL, 0, { 0 }, "Usage: STATUS\n", cmd_def },
    { "DELIVER", NULL, 0, { 0 }, "Usage: DELIVER\n", cmd_def },
    { "CLAIM", NULL, 0, { 0 }, "Usage: CLAIM\n", cmd_def },
};

static const int numCommands = sizeof(commands) / sizeof(commands[0]);

static int isNumeric(const char *s) {
    if (s == NULL || *s == '\0') {
        return 0;
    }
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return 0;
        }
    }
    return 1;
}

static int tokenizeLine(char *line, char *argv[], int maxTokens) {
    int argc = 0;
    char *token = strtok(line, " \t");

    while (token != NULL && argc < maxTokens) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    return argc;
}
static Command *findCommand(char *tokens[], int tokenCount, int *nameWords) {
    for (int i = 0; i < numCommands; i++) {
        if (commands[i].word2 != NULL) {
            if (tokenCount >= 2 &&
                strcasecmp(tokens[0], commands[i].word1) == 0 &&
                strcasecmp(tokens[1], commands[i].word2) == 0) {
                *nameWords = 2;
                return &commands[i];
            }
        }
    }
    for (int i = 0; i < numCommands; i++) {
        if (commands[i].word2 == NULL) {
            if (tokenCount >= 1 && strcasecmp(tokens[0], commands[i].word1) == 0) {
                *nameWords = 1;
                return &commands[i];
            }
        }
    }
    return NULL;
}

static void processLine(char *tokens[], int tokenCount) {
    if (tokenCount == 0) {
        return;
    }

    int nameWords = 0;
    Command *cmd = findCommand(tokens, tokenCount, &nameWords);

    if (cmd == NULL) {
        write(STDOUT_FILENO, "Unknown command\n", strlen("Unknown command\n"));
        return;
    }

    char **args = &tokens[nameWords];
    int givenArgs = tokenCount - nameWords;

    if (givenArgs != cmd->numArgs) {
        write(STDOUT_FILENO, cmd->usage, strlen(cmd->usage));
        return;
    }

    for (int i = 0; i < givenArgs; i++) {
        if (cmd->argKinds[i] == ARG_NUMERIC && !isNumeric(args[i])) {
            write(STDOUT_FILENO, cmd->usage, strlen(cmd->usage));
            return;
        }
    }

    cmd->handler(givenArgs, args);
    write(STDOUT_FILENO, "Command OK\n", strlen("Command OK\n"));
}