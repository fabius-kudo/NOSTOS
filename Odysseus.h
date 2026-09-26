#ifndef CODE_ODYSSEUS_H
#define CODE_ODYSSEUS_H

#define MAX_NAME 50
#define MAX_PATH 256
#define MAX_FOOD 20
#define IP_LENGTH 16

#define MAX_ARGS 4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>



typedef struct {
    char product[MAX_NAME];
    float quantity;
} Food;

typedef struct {
    char name[MAX_NAME];
    char file_path[MAX_PATH];
    char ithaca_ip[IP_LENGTH];
    int ithaca_port;
    char island_name[MAX_NAME];
    char island_ip[IP_LENGTH];
    int islandPort;
    int initial_money; //maybe should be float
    int num_food_items;
    Food food_supplies[MAX_FOOD];
} Odysseus;

//  --- terminal structs ---

typedef int (*CommandHandler)(int argc, char *argv[]);  

typedef struct {
    const char     *name;       
    int             numArgs;   //excluding self
    const char     *usage;     //message for args
    CommandHandler  handler;
} Command;


#endif //CODE_ODYSSEUS_H
