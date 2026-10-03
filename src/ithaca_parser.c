/***********************************************
 *
 * @File : ithaca_parser.c
 * @Purpose : Parsing functions for Ithaca data
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "ithaca_parser.h"
#include "custom_dynamic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

Ithaca parseIthaca(char *config_path) {
    Ithaca ithaca;
    memset(&ithaca, 0, sizeof(ithaca));

    int fd = open(config_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed opening config file\n", strlen("Error: failed opening config file\n"));
        exit(EXIT_FAILURE);
    }

    char *buf = readDynamic(fd, 0);
    close(fd);
    if (!buf) {
        write(STDERR_FILENO, "Error: malloc failed opening config file\n", strlen("Error: malloc failed opening config file\n"));
        exit(EXIT_FAILURE);
    }

    ithaca.serverName = dupString(strtok(buf, " \n"));
    ithaca.path       = dupString(strtok(NULL, " \n"));
    ithaca.ip_address = dupString(strtok(NULL, " \n"));
    ithaca.port       = atoi(strtok(NULL, " \n"));

    free(buf);
    return ithaca;
}

void freeIthaca(Ithaca *ithaca) {
    free(ithaca->serverName);
    free(ithaca->path);
    free(ithaca->ip_address);
    memset(ithaca, 0, sizeof(*ithaca));
}

Voyage *parseVoyages(char *voyages_path, int *num_voyages) {
    int fd = open(voyages_path, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: failed opening voyages file\n", strlen("Error: failed opening voyages file\n"));
        exit(EXIT_FAILURE);
    }

    char *buf = readDynamic(fd, 0);
    close(fd);
    if (!buf) {
        write(STDERR_FILENO, "Error: malloc failed opening voyages file\n", strlen("Error: malloc failed opening voyages file\n"));
        exit(EXIT_FAILURE);
    }

    // First pass: count lines, using a copy since strtok is destructive
    char *bufCopy = dupString(buf);
    if (!bufCopy) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    int count = 0;
    char *line = strtok(bufCopy, "\n");
    while (line != NULL) {
        count++;
        line = strtok(NULL, "\n");
    }
    free(bufCopy);

    Voyage *voyages = malloc(sizeof(Voyage) * count);
    if (!voyages && count > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    // Second pass: parse each line into a Voyage
    char *tok = strtok(buf, " \n"); 
    for (int i = 0; i < count; i++) {
        voyages[i].object      = dupString(tok);
        voyages[i].file        = dupString(strtok(NULL, " \n"));
        voyages[i].destination = dupString(strtok(NULL, " \n"));
        voyages[i].reward      = atoi(strtok(NULL, " \n"));
        tok = strtok(NULL, " \n");
    }

    free(buf);
    *num_voyages = count;
    return voyages;
}

void freeVoyages(Voyage *voyages, int num_voyages) {
    for (int i = 0; i < num_voyages; i++) {
        free(voyages[i].object);
        free(voyages[i].file);
        free(voyages[i].destination);
    }
    free(voyages);
}

//TEST FUNCTION, ILLEGAL TODO:REMOVE
void printIthaca(const Ithaca *ithaca) {
    printf("--- Ithaca ---\n");
    printf("serverName: [%s]\n", ithaca->serverName);
    printf("path:       [%s]\n", ithaca->path);
    printf("address:    [%s]:%d\n", ithaca->ip_address, ithaca->port);
    printf("--------------\n");
    fflush(stdout);
}

//TEST FUNCTION, ILLEGAL TODO:REMOVE
void printVoyages(const Voyage *voyages, int num_voyages) {
    printf("--- Voyages (%d) ---\n", num_voyages);
    for (int i = 0; i < num_voyages; i++) {
        printf("  [%d] object=[%s] file=[%s] destination=[%s] reward=%d\n",
               i,
               voyages[i].object,
               voyages[i].file,
               voyages[i].destination,
               voyages[i].reward);
    }
    printf("--------------------\n");
    fflush(stdout);
}