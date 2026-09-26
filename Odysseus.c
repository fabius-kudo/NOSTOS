#include "Odysseus.h"
#include "custom_string.h"

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
    //od.food_supplies = malloc(sizeof(Food) * od.num_food_items);  TODO fix
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



int main(int argc, char *argv[]) {
    char *line;
    
    if (argc != 2) {
        //TODO: add error message
        write(STDOUT_FILENO,"ERROR\n",strlen("ERROR\n"));
        exit(EXIT_FAILURE);
    }

    char *config_path = argv[1];

    //Odysseus odysseus = parseOdysseus(config_path);

    while ((line = readLineDynamic(STDIN_FILENO)) != NULL) {
        char *argv[MAX_ARGS];
        int argc = tokenizeLine(line, argv, MAX_ARGS);

        processLine(argv, argc);

        free(line);
    }

    return 0;
}