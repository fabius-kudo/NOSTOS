#ifndef CODE_PORT_CONTROL_H
#define CODE_PORT_CONTROL_H

#include <pthread.h>

// The semaphore is kept here as its raw id.
typedef struct {
    pthread_mutex_t stMutex;    // protects the counters below
    int nWaitSemId;             // semaphore (init 0) where queued ships sleep, FIFO
    int nDocks;                 // capacity, from the island config
    int nDocked;                // docks in use (a hand-off keeps the dock counted)
    int nQueued;                // ships sleeping on the semaphore
    int nClosing;
} tPort;

int  portInit(tPort *pstPort, int nDocks);
int  portArrive(tPort *pstPort);    // 1 = docked now, 0 = queued: call portWaitTurn
int  portWaitTurn(tPort *pstPort);  // 0 = docked, -1 = port closing
void portLeave(tPort *pstPort);     // free the dock, or hand it to the oldest waiter
void portClose(tPort *pstPort);     // wake every waiter (Ctrl+C)
void portDestroy(tPort *pstPort);   // removes the semaphore from the kernel

#endif