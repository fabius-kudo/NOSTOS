#include <errno.h>
#include "semaphore_v2.h"       // only included here: it defines functions in the header
#include "port_control.h"

static int waitSem(tPort *pstPort) {
    semaphore stSem;
    int nRet;

    stSem.shmid = pstPort->nWaitSemId;
    do {
        nRet = SEM_wait(&stSem);
    } while (nRet < 0 && errno == EINTR);
    return nRet;
}

static int signalSem(tPort *pstPort) {
    semaphore stSem;

    stSem.shmid = pstPort->nWaitSemId;
    return SEM_signal(&stSem);
}

int portInit(tPort *pstPort, int nDocks) {
    semaphore stSem;

    pstPort->nDocks = nDocks;
    pstPort->nDocked = 0;
    pstPort->nQueued = 0;
    pstPort->nClosing = 0;

    if (SEM_constructor(&stSem) < 0) {
        return -1;
    }
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

int portArrive(tPort *pstPort) {
    int nDocked = 0;

    pthread_mutex_lock(&pstPort->stMutex);
    if (pstPort->nDocked < pstPort->nDocks) {
        pstPort->nDocked++;
        nDocked = 1;
    } else {
        pstPort->nQueued++;             // the caller must now call portWaitTurn
    }
    pthread_mutex_unlock(&pstPort->stMutex);
    return nDocked;
}

int portWaitTurn(tPort *pstPort) {
    int nRet;

    // If the signal came first the count is 1 and this returns immediately
    if (waitSem(pstPort) < 0) {
        return -1;
    }
    pthread_mutex_lock(&pstPort->stMutex);
    nRet = pstPort->nClosing ? -1 : 0;
    pthread_mutex_unlock(&pstPort->stMutex);
    return nRet;
}

void portLeave(tPort *pstPort) {
    int nHandOff = 0;

    pthread_mutex_lock(&pstPort->stMutex);
    if (pstPort->nQueued > 0) {
        pstPort->nQueued--;             // the dock goes to the oldest waiter, nDocked unchanged
        nHandOff = 1;
    } else {
        pstPort->nDocked--;
    }
    pthread_mutex_unlock(&pstPort->stMutex);

    if (nHandOff) {
        signalSem(pstPort);
    }
}

void portClose(tPort *pstPort) {
    int nWake;

    pthread_mutex_lock(&pstPort->stMutex);
    pstPort->nClosing = 1;
    nWake = pstPort->nQueued;
    pstPort->nQueued = 0;
    pthread_mutex_unlock(&pstPort->stMutex);

    for (int i = 0; i < nWake; i++) {
        signalSem(pstPort);
    }
}

void portDestroy(tPort *pstPort) {
    semaphore stSem;

    stSem.shmid = pstPort->nWaitSemId;
    SEM_destructor(&stSem);
    pthread_mutex_destroy(&pstPort->stMutex);
}