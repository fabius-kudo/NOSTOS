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
#include "custom_io.h"

// Constant Declarations
#define MAX_ARGS 4

typedef enum {
    ARG_NONE,
    ARG_NUMERIC,
    ARG_TEXT
} tArgKind;

typedef enum {
    CMD_UNKNOWN,
    CMD_CONNECT_ITHACA,
    CMD_LIST_VOYAGES,
    CMD_ACCEPT,
    CMD_SAIL,
    CMD_MAP,
    CMD_LIST_MARKET,
    CMD_BUY,
    CMD_BUY_MAP,
    CMD_SELL,
    CMD_STATUS,
    CMD_DELIVER,
    CMD_CLAIM
} tCmdId;

void processLine(tShip *pstShip, char *ppsTokens[], int nTokenCount);
int tokenizeLine(char *psLine, char *ppsArgv[], int nMaxTokens);

#endif