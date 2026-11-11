/***********************************************
 *
 * @File : port_control.h
 * @Purpose : Header file for the island port control
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 11/11/26
 *
 ***********************************************/
#ifndef CODE_PORT_CONTROL_H
#define CODE_PORT_CONTROL_H

// System Includes
#include <pthread.h>

// Custom Type Definitions
typedef enum { PORT_OUT, PORT_QUEUED, PORT_DOCKED, PORT_GONE } tPortState;

// What the port knows about one ship. It lives inside the session of the
// ship and it is also the node of the waiting list.
typedef struct tPortShip {
    int nFd;                    // socket of the ship, to send it DOCKED
    char *psName;               // name of the ship (owned by its session)
    tPortState eState;          // only read or written with the port mutex locked
    struct tPortShip *pstNext;  // next ship in the waiting list
} tPortShip;

typedef struct {
    pthread_mutex_t stMutex;    // protects everything below and every tPortShip
    int nDocks;                 // capacity, from the island config
    int nDocked;                // docks in use (a hand-off keeps the dock counted)
    tPortShip *pstFirst;        // waiting list: the oldest ship, next to dock
    tPortShip *pstLast;         // waiting list: the newest ship
} tPort;

//Prototypes
int portInit(tPort *pstPort, int nDocks);
void portShipInit(tPortShip *pstShip, int nFd);
tPortState portGetState(tPort *pstPort, tPortShip *pstShip);
int portArrive(tPort *pstPort, tPortShip *pstShip, char *psName);
tPortState portLeave(tPort *pstPort, tPortShip *pstShip, char **ppsNextName);
void portDestroy(tPort *pstPort);

#endif