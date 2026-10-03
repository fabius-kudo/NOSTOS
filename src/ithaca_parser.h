#ifndef CODE_ITHACA_PARSER_H
#define CODE_ITHACA_PARSER_H

typedef struct {
    char *serverName;
    char *path;
    char *ip_address;
    int port;
} Ithaca;

typedef struct {
    char *object;
    char *file;
    char *destination;
    int reward;
} Voyage;

Ithaca parseIthaca(char *config_path);
void freeIthaca(Ithaca *ithaca);

Voyage *parseVoyages(char *voyages_path, int *num_voyages);
void freeVoyages(Voyage *voyages, int num_voyages);

void printIthaca(const Ithaca *ithaca);
void printVoyages(const Voyage *voyages, int num_voyages);

#endif //CODE_ITHACA_PARSER_H