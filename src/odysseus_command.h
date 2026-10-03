/***********************************************
 *
 * @File : odysseus_command.h
 * @Purpose : Header file for command handling in the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_ODYSSEUS_COMMAND_H
#define CODE_ODYSSEUS_COMMAND_H

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// Constant Declarations
#define MAX_ARGS 4

// Custom Type Definitions
typedef int (*tCommandHandler)(int nArgc, char *ppsArgv[]);

typedef enum {
    ARG_NONE,
    ARG_NUMERIC,
    ARG_TEXT
} tArgKind;

typedef struct {
    const char *psWord1;
    const char *psWord2;
    int nNumArgs;
    tArgKind aArgKinds[2];
    const char *psUsage;
    tCommandHandler fHandler;
} tCommand;

void processLine(char *ppsTokens[], int nTokenCount);
int tokenizeLine(char *psLine, char *ppsArgv[], int nMaxTokens);

#endif //CODE_ODYSSEUS_COMMAND_H