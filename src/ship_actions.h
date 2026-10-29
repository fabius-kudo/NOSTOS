#ifndef CODE_SHIP_ACTIONS_H
#define CODE_SHIP_ACTIONS_H

#include "ship_config.h"
#include "protocol.h"
#include "socket_utils.h"
#include <poll.h>
#include <time.h>

#define RATION_INTERVAL_MS 5000
#define RATION_KG 7

int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]);
int cmdListVoyages(tShip *pContext, int nArgc, char *ppsArgv[]);
int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]);
int cmdSail(tShip *pstShip, int nArgc, char *ppsArgv[]);

void releaseShip(tShip *pstShip);

#endif