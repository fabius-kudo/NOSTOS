/***********************************************
 *
 * @File : commands.h
 * @Purpose : Header file for command handling in the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_COMMANDS_H
#define CODE_COMMANDS_H

// System Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

//prj includes
#include "ship_actions.h"
#include "ship_config.h"

// Constant Declarations
#define MAX_ARGS 4

// Custom Type Definitions
typedef int (*tCommandHandler)(void *context, int nArgc, char *ppsArgv[]);

typedef enum {
    ARG_NONE,
    ARG_NUMERIC,
    ARG_TEXT
} tArgKind;

typedef enum {
    CMD_CONNECT_ITHACA,
    CMD_LIST_VOYAGES,
    CMD_ACCEPT,
    CMD_SAIL,
    CMD_MAP,
    CMD_LIST_MARKET,
    CMD_BUY,
    CMD_SELL,
    CMD_STATUS,
    CMD_DELIVER,
    CMD_CLAIM
} tCmdId;

typedef struct {
    const char *psWord1;
    const char *psWord2;       // NULL for one-word commands
    int nNumArgs;
    int aArgKinds[2];          // ARG_NUMERIC / ARG_TEXT (keep your own type)
    const char *psUsage;
    tCmdId id;                 // replaces the function pointer
} tCommand;

void processLine(tShip *pstShip, char *ppsTokens[], int nTokenCount);
int tokenizeLine(char *psLine, char *ppsArgv[], int nMaxTokens);

#endif