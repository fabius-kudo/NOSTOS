#ifndef CODE_ISLAND_SERVICE_H
#define CODE_ISLAND_SERVICE_H

#include "port_control.h"
#include "protocol.h"
#include "socket_utils.h"
#include "island_stock.h"

typedef enum { SHIP_NONE, SHIP_QUEUED, SHIP_DOCKED, SHIP_GONE } tShipState;

typedef struct {
    int nFd;
    tPort *pstPort;             // shared: docks and queue
    tStock *pstStock;           // shared: protected by its own mutex
    const tIsland *pstIsland;   // shared, read-only after startup (routes for BUY MAP)
    const char *psIslandName;
    char *psShipName;           // heap copy, set on ARRIVE
    tShipState eState;
} tIslandSession;

void serveShip(tIslandSession *pstSession);

#endif