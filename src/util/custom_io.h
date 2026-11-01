/***********************************************
 *
 * @File : custom_dynamic.h
 * @Purpose : Header file for th e dynamic memory management functions
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CUSTOM_DYNAMIC_H
#define CUSTOM_DYNAMIC_H

// System Includes
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define printF(x) write(STDOUT_FILENO, x, strlen(x))
#define printE(x) write(STDERR_FILENO, x, strlen(x))

char *readDynamic(int nFd, int nUntilNewline);   // reads until '\n' or EOF
char *dupString(const char *psSource);  // heap copy of a string
void printAndFree(char *psMsg);

#endif  //CUSTOM_DYNAMIC_H