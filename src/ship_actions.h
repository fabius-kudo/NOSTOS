#ifndef CODE_SHIP_ACTIONS_H
#define CODE_SHIP_ACTIONS_H

#include "ship_config.h"

static int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]);
static int cmdListVoyages(tShip *pContext, int nArgc, char *ppsArgv[]);
static int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]);

#endif