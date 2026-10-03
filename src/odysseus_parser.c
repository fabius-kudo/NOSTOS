/***********************************************
 *
 * @File : odysseus_parser.c
 * @Purpose : Parser for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "odysseus_parser.h"
#include "custom_dynamic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

Odysseus parseOdysseus(char * config_path) {
    Odysseus od;
    memset(&od, 0, sizeof(od)); //like malloc and all pointers NULL

    int fd = open(config_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed opening config file\n", 34);
        exit(EXIT_FAILURE);
    }

    //Read whole file into buffer
    char *buf = readDynamic(fd,0);
    close(fd);
    if (!buf) {
        write(STDERR_FILENO, "Error: malloc failed opening config file\n", 40);
        exit(EXIT_FAILURE);
    }

    od.name = dupString(strtok(buf, " \n"));
    od.file_path = dupString(strtok(NULL, " \n"));
    od.ithaca_ip = dupString(strtok(NULL, " \n"));
    od.ithaca_port = atoi(strtok(NULL, " \n"));

    od.island_name = dupString(strtok(NULL, " \n"));
    od.island_ip = dupString(strtok(NULL, " \n"));
    od.island_port = atoi(strtok(NULL, " \n"));

    od.initial_money = atoi(strtok(NULL, " \n"));
    od.num_food_items = atoi(strtok(NULL, " \n"));

    od.food_supplies = malloc(sizeof(Food) * od.num_food_items);
    if (!od.food_supplies && od.num_food_items > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", 21);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < od.num_food_items; i++) {
        od.food_supplies[i].product  = dupString(strtok(NULL, " \n"));
        od.food_supplies[i].quantity = atoi(strtok(NULL, " \n"));
    }

    free(buf);   //free after copies made
    return od;

}

void freeOdysseus(Odysseus *od) {
    free(od->name);
    free(od->file_path);
    free(od->ithaca_ip);
    free(od->island_name);
    free(od->island_ip);
    for (int i = 0; i < od->num_food_items; i++) {
        free(od->food_supplies[i].product);
    }
    free(od->food_supplies);
    memset(od, 0, sizeof(*od));            // no dangling pointers afterwards
}

//TEST FUNCTION, ILLEGAL TODO:REMOVE
void printOdysseus(const Odysseus *od) {
    printf("--- Odysseus ---\n");
    printf("name:          [%s]\n", od->name);
    printf("file_path:     [%s]\n", od->file_path);
    printf("ithaca:        [%s]:%d\n", od->ithaca_ip, od->ithaca_port);
    printf("island:        [%s] [%s]:%d\n", od->island_name, od->island_ip, od->island_port);
    printf("initial_money: %d\n", od->initial_money);
    printf("food items:    %d\n", od->num_food_items);
    for (int i = 0; i < od->num_food_items; i++) {
        printf("  [%d] [%s] x %d\n", i, od->food_supplies[i].product, od->food_supplies[i].quantity);
    }
    printf("----------------\n");
    fflush(stdout);
}