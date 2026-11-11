/***********************************************
 *
 * @File : island_service.h
 * @Purpose : Header file for the island service
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/
#ifndef CODE_ISLAND_SERVICE_H
#define CODE_ISLAND_SERVICE_H

// System Includes
#include "port_control.h"
#include "protocol.h"
#include "socket_utils.h"
#include "island_stock.h"
#include "custom_io.h"
#include "port_config.h"

// Custom Type Definitions
typedef struct {
    int nFd;
    tPort *pstPort;             // shared: docks and waiting list
    tStock *pstStock;           // shared: protected by its own mutex
    const tIsland *pstIsland;   // shared, read-only after startup (routes for BUY MAP)
    const char *psIslandName;
    char *psShipName;           // heap copy, set on ARRIVE
    tPortShip stShip;           // state of this ship in the port (use the port functions)
} tIslandSession;

//Prototypes
void serveShip(tIslandSession *pstSession);

#endif