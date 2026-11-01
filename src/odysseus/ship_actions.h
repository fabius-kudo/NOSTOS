/***********************************************
 *
 * @File : ship_actions.h
 * @Purpose : Header file for the Odysseus terminal command handlers.
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 1/11/26
 *
 ***********************************************/

#ifndef CODE_SHIP_ACTIONS_H
#define CODE_SHIP_ACTIONS_H

// System Includes
#include <strings.h>
#include <poll.h>
#include <time.h>
#include <ctype.h>

// Project Includes
#include "socket_utils.h"
#include "ship_config.h"
#include "protocol.h"
#include "SPHRAGIS Library-20260917/sphragis.h"
#include "custom_io.h"

// Constants
#define RATION_INTERVAL_MS 5000     // while waiting offshore, one ration is eaten every full 5 s
#define RATION_KG 7                 // kilograms of food consumed per ration
#define MAP_PRICE 50                // fixed price of any island's map, in gold

// --- Ithaca ---
int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]);
int cmdListVoyages(tShip *pContext, int nArgc, char *ppsArgv[]);
int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]);

// --- Sailing ---
int cmdSail(tShip *pstShip, int nArgc, char *ppsArgv[]);

// --- Island market ---
int cmdListMarket(tShip *pstShip, int nArgc, char *ppsArgv[]);
int cmdBuy(tShip *pstShip, int nArgc, char *ppsArgv[]);
int cmdSell(tShip *pstShip, int nArgc, char *ppsArgv[]);
int cmdBuyMap(tShip *pstShip, int nArgc, char *ppsArgv[]);

// --- Local commands (no frames sent) ---
int cmdMap(tShip *pstShip, int nArgc, char *ppsArgv[]);
int cmdStatus(tShip *pstShip, int nArgc, char *ppsArgv[]);

void releaseShip(tShip *pstShip);

#endif