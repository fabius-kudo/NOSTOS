#ifndef CODE_ITHACA_PARSER_H
#define CODE_ITHACA_PARSER_H

#define MAX_NAME 50
#define MAX_PATH 256
#define IP_LENGTH 16

typedef struct {
    char serverName[MAX_NAME];
    char path[MAX_PATH];
    char ip_address[IP_LENGTH];
    int port;
} Ithaca;

typedef struct {
    char object[MAX_NAME];
    char file[MAX_PATH];
    char destination[MAX_NAME];
    int reward;
} Voyage;

Ithaca parseIthaca(char *config_path);
Voyage *parseVoyages(char *voyages_path, int *num_voyages);

#endif //CODE_ITHACA_PARSER_H