#ifndef CODE_ODYSSEUS_PARSER_H
#define CODE_ODYSSEUS_PARSER_H

typedef struct {
    char *product;
    int quantity;
} Food;

typedef struct {
    char *name;
    char *file_path;

    char *ithaca_ip;
    int  ithaca_port;

    char *island_name;
    char *island_ip;
    int  island_port;

    int  initial_money;
    int  num_food_items;
    Food *food_supplies;
} Odysseus;

Odysseus parseOdysseus(char *config_path);
void freeOdysseus(Odysseus *od);

void printOdysseus(const Odysseus *od);

#endif //CODE_ODYSSEUS_PARSER_H