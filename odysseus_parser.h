#ifndef CODE_ODYSSEUS_PARSER_H
#define CODE_ODYSSEUS_PARSER_H

#define MAX_NAME 100
#define MAX_PATH 256
#define MAX_IP 16

typedef struct {
    char product[MAX_NAME];
    int quantity;
} Food;

typedef struct {
    char name[MAX_NAME];
    char file_path[MAX_PATH];

    char ithaca_ip[MAX_IP];
    int  ithaca_port;

    char island_name[MAX_NAME];
    char island_ip[MAX_IP];
    int  island_port;

    int  initial_money;
    int  num_food_items;
    Food *food_supplies;
} Odysseus;

Odysseus parseOdysseus(char *config_path);
void freeOdysseus(Odysseus *od);

#endif //CODE_ODYSSEUS_PARSER_H