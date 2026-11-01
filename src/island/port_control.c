/***********************************************
 *
 * @File : port_control.c
 * @Purpose : Dock control of an island: limited berths and a FIFO queue of waiting ships
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/

#include "port_control.h"
// included only here (not in all that include port_control.h)
#include "semaphore_v2.h"


/***********************************************
 *
 * @Name: waitSem
 * @Def: Waits on the port semaphore, retrying if a signal interrupts the wait.
 * @Arg: In: pstPort pointer to the port
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
static int waitSem(tPort *pstPort) {
    semaphore stSem;
    int nRet;

    // The port only stores the semaphore id; rebuild the library struct from it
    stSem.shmid = pstPort->nWaitSemId;
    do {
        nRet = SEM_wait(&stSem);
    } while (nRet < 0 && errno == EINTR);
    return nRet;
}

/***********************************************
 *
 * @Name: signalSem
 * @Def: Signals the port semaphore, waking the oldest sleeping ship.
 * @Arg: In: pstPort pointer to the port
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
static int signalSem(tPort *pstPort) {
    semaphore stSem;

    stSem.shmid = pstPort->nWaitSemId;
    return SEM_signal(&stSem);
}

/***********************************************
 *
 * @Name: portInit
 * @Def: Initializes the port: counters, mutex and the waiting semaphore (value 0).
 * @Arg: In/Out: pstPort pointer to the port to initialize
 *       In: nDocks number of berths of the island
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
int portInit(tPort *pstPort, int nDocks) {
    semaphore stSem;

    pstPort->nDocks = nDocks;
    pstPort->nDocked = 0;
    pstPort->nQueued = 0;
    pstPort->nClosing = 0;

    if (SEM_constructor(&stSem) < 0) {
        return -1;
    }
    // Value 0: nobody passes the semaphore until a leaving ship signals it
    if (SEM_init(&stSem, 0) < 0) {
        SEM_destructor(&stSem);
        return -1;
    }
    pstPort->nWaitSemId = stSem.shmid;

    if (pthread_mutex_init(&pstPort->stMutex, NULL) != 0) {
        SEM_destructor(&stSem);
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Name: portArrive
 * @Def: A ship asks for a berth. It docks at once if one is free, otherwise
 *       it is counted in the queue.
 * @Arg: In/Out: pstPort pointer to the port
 * @Ret: Returns 1 if the ship docked, 0 if it must call portWaitTurn.
 *
 ***********************************************/
int portArrive(tPort *pstPort) {
    int nDocked = 0;

    // check and upadte in one step (so 2 dont take last)
    pthread_mutex_lock(&pstPort->stMutex);
    if (pstPort->nDocked < pstPort->nDocks) {
        pstPort->nDocked++;
        nDocked = 1;
    } else {
        // The caller must now call portWaitTurn
        pstPort->nQueued++;
    }
    pthread_mutex_unlock(&pstPort->stMutex);
    return nDocked;
}

/***********************************************
 *
 * @Name: portWaitTurn
 * @Def: Sleeps until a berth is handed over to this ship, or the port closes.
 * @Arg: In/Out: pstPort pointer to the port
 * @Ret: Returns 0 if the ship has a berth, -1 if the port is closing or on error.
 *
 ***********************************************/
int portWaitTurn(tPort *pstPort) {
    int nRet;

    // The mutex is not held while sleeping, or nobody else could use the port.
    // If the signal came first the count is 1 and this returns immediately.
    if (waitSem(pstPort) < 0) {
        return -1;
    }
    pthread_mutex_lock(&pstPort->stMutex);
    nRet = pstPort->nClosing ? -1 : 0;
    pthread_mutex_unlock(&pstPort->stMutex);
    return nRet;
}

/***********************************************
 *
 * @Name: portLeave
 * @Def: A docked ship frees its berth: it is handed to the oldest queued ship,
 *       or becomes free if nobody is waiting.
 * @Arg: In/Out: pstPort pointer to the port
 * @Ret: None.
 *
 ***********************************************/
void portLeave(tPort *pstPort) {
    int nHandOff = 0;

    pthread_mutex_lock(&pstPort->stMutex);
    if (pstPort->nQueued > 0) {
        //berth goes to the oldest waiter, so nDocked stays the same
        pstPort->nQueued--;
        nHandOff = 1;
    } else {
        pstPort->nDocked--;
    }
    pthread_mutex_unlock(&pstPort->stMutex);

    //Signal after unlocking: next ship can lock the mutex right away
    if (nHandOff) {
        signalSem(pstPort);
    }
}

/***********************************************
 *
 * @Name: portClose
 * @Def: Closes the port and wakes every queued ship so none stays asleep forever.
 * @Arg: In/Out: pstPort pointer to the port
 * @Ret: None.
 *
 ***********************************************/
void portClose(tPort *pstPort) {
    int nWake;

    // Woken ships read nClosing and know they did not get a berth
    pthread_mutex_lock(&pstPort->stMutex);
    pstPort->nClosing = 1;
    nWake = pstPort->nQueued;
    pstPort->nQueued = 0;
    pthread_mutex_unlock(&pstPort->stMutex);

    // One signal for each sleeping ship
    for (int i = 0; i < nWake; i++) {
        signalSem(pstPort);
    }
}

/***********************************************
 *
 * @Name: portDestroy
 * @Def: Removes the semaphore from the system and destroys the mutex.
 * @Arg: In/Out: pstPort pointer to the port
 * @Ret: None.
 *
 ***********************************************/
void portDestroy(tPort *pstPort) {
    semaphore stSem;

    //semaphore lives in the kernel, destroyed explicitly
    stSem.shmid = pstPort->nWaitSemId;
    SEM_destructor(&stSem);
    pthread_mutex_destroy(&pstPort->stMutex);
}