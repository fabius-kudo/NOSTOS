/***********************************************
 *
 * @File : port_control.h
 * @Purpose : Header file for the island port control
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/
#ifndef CODE_PORT_CONTROL_H
#define CODE_PORT_CONTROL_H

 // System Includes
#include <pthread.h>
#include <errno.h>

// Custom Type Definitions
typedef struct {
    pthread_mutex_t stMutex;    // protects the counters below
    int nWaitSemId;             // semaphore (init 0) where queued ships sleep, FIFO
    int nDocks;                 // capacity, from the island config
    int nDocked;                // docks in use (a hand-off keeps the dock counted)
    int nQueued;                // ships sleeping on the semaphore
    int nClosing;
} tPort;

//Prototypes
int  portInit(tPort *pstPort, int nDocks);
int  portArrive(tPort *pstPort);    // 1 = docked now, 0 = queued: call portWaitTurn
int  portWaitTurn(tPort *pstPort);  // 0 = docked, -1 = port closing
void portLeave(tPort *pstPort);     // free the dock, or hand it to the oldest waiter
void portClose(tPort *pstPort);     // wake every waiter (Ctrl+C)
void portDestroy(tPort *pstPort);   // removes the semaphore from the kernel

#endif