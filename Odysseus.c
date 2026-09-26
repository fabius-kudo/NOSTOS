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
    { "add",    2, "Usage: add <name> <value>\n",   cmd_def },
    { "remove",    0, "Usage: remove\n",   cmd_def }
};

static const int numCommands = sizeof(commands) / sizeof(commands[0]);

static int tokenizeLine(char *line, char *argv[], int maxTokens) {
    int argc = 0;
    char *token = strtok(line, " \t");

    while (token != NULL && argc < maxTokens) {
        argv[argc++] = token;
        token = strtok(NULL, " \t");
    }
    return argc;
}

static Command *findCommand(const char *name) {
    for (int i = 0; i < numCommands; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            return &commands[i];
        }
    }
    return NULL;
}

static void processLine(char *argv[], int argc){
    if (argc == 0){         //blank line
        return;
    }

    Command *cmd = findCommand(argv[0]);

    if (cmd == NULL){
        write(STDOUT_FILENO,"Unknown command\n",strlen("Unknown command\n"));
        return;
    }

    int givenArgs = argc - 1; //exclude commands as arg

    if (givenArgs != cmd->numArgs){
        write(STDOUT_FILENO,cmd->usage,strlen(cmd->usage));
        return;
    }

    cmd->handler(argc, argv); //add return val check maybe
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