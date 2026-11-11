/***********************************************
 *
 * @File : port_control.c
 * @Purpose : Dock control of an island: limited docks and a FIFO list of waiting ships
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 31/10/26
 *
 ***********************************************/

#include "port_control.h"
#include "protocol.h"
#include "custom_io.h"

/***********************************************
 *
 * @Name: takeFirst
 * @Def: Takes the oldest ship out of the waiting list.
 * @Arg: In/Out: pstPort pointer to the port (the caller must hold its mutex)
 * @Ret: The ship, or NULL if nobody is waiting.
 *
 ***********************************************/
tPortShip *takeFirst(tPort *pstPort) {
    tPortShip *pstShip = pstPort->pstFirst;

    if (pstShip != NULL) {
        pstPort->pstFirst = pstShip->pstNext;
        if (pstPort->pstFirst == NULL) {
            pstPort->pstLast = NULL;
        }
        pstShip->pstNext = NULL;
    }
    return pstShip;
}

/***********************************************
 *
 * @Name: removeWaiting
 * @Def: Takes a ship out of the waiting list, wherever it is.
 * @Arg: In/Out: pstPort pointer to the port (the caller must hold its mutex)
 *       In/Out: pstShip ship to remove
 * @Ret: None.
 *
 ***********************************************/
void removeWaiting(tPort *pstPort, tPortShip *pstShip) {
    tPortShip *pstPrev = NULL;
    tPortShip *pstCur = pstPort->pstFirst;

    while (pstCur != NULL && pstCur != pstShip) {
        pstPrev = pstCur;
        pstCur = pstCur->pstNext;
    }
    if (pstCur == NULL) {
        return;
    }
    if (pstPrev == NULL) {
        pstPort->pstFirst = pstShip->pstNext;
    } else {
        pstPrev->pstNext = pstShip->pstNext;
    }
    if (pstPort->pstLast == pstShip) {
        pstPort->pstLast = pstPrev;
    }
    pstShip->pstNext = NULL;
}

/***********************************************
 *
 * @Name: portInit
 * @Def: Initializes the port: all docks free, nobody waiting.
 * @Arg: Out: pstPort pointer to the port to initialize
 *       In: nDocks number of docks of the island
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
int portInit(tPort *pstPort, int nDocks) {
    pstPort->nDocks = nDocks;
    pstPort->nDocked = 0;
    pstPort->pstFirst = NULL;
    pstPort->pstLast = NULL;

    if (pthread_mutex_init(&pstPort->stMutex, NULL) != 0) {
        return -1;
    }
    return 0;
}

/***********************************************
 *
 * @Name: portShipInit
 * @Def: Prepares the port data of a ship that has just connected.
 * @Arg: Out: pstShip port data of the ship
 *       In: nFd socket of the ship
 * @Ret: None.
 *
 ***********************************************/
void portShipInit(tPortShip *pstShip, int nFd) {
    pstShip->nFd = nFd;
    pstShip->psName = NULL;
    pstShip->eState = PORT_OUT;
    pstShip->pstNext = NULL;
}

/***********************************************
 *
 * @Name: portGetState
 * @Def: Reads the state of a ship. Another thread can change it (when a dock
 *       is handed over), so it is always read with the mutex locked.
 * @Arg: In/Out: pstPort pointer to the port
 *       In: pstShip port data of the ship
 * @Ret: The state of the ship.
 *
 ***********************************************/
tPortState portGetState(tPort *pstPort, tPortShip *pstShip) {
    tPortState eState;

    pthread_mutex_lock(&pstPort->stMutex);
    eState = pstShip->eState;
    pthread_mutex_unlock(&pstPort->stMutex);
    return eState;
}

/***********************************************
 *
 * @Name: portArrive
 * @Def: A ship asks for a dock. If one is free it takes it and is told
 *       DOCKED; otherwise it joins the end of the waiting list and is told WAIT.
 * @Arg: In/Out: pstPort pointer to the port
 *       In/Out: pstShip port data of the ship
 *       In: psName name of the ship (it stays owned by the caller)
 * @Ret: Returns 1 if the ship docked, 0 if it is waiting, -1 if the answer
 *       could not be sent (the ship must then leave with portLeave).
 *
 ***********************************************/
int portArrive(tPort *pstPort, tPortShip *pstShip, char *psName) {
    int nDocked = 0;
    int nSent;

    // The check, the update and the answer are one locked step: two ships
    // cannot take the last dock, and no DOCKED can overtake this WAIT
    pthread_mutex_lock(&pstPort->stMutex);
    pstShip->psName = psName;
    if (pstPort->nDocked < pstPort->nDocks) {
        pstPort->nDocked++;
        pstShip->eState = PORT_DOCKED;
        nDocked = 1;
        nSent = sendTextFrame(pstShip->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "DOCKED");
    } else {
        // Added at the end: the list keeps the order of arrival
        pstShip->pstNext = NULL;
        if (pstPort->pstLast == NULL) {
            pstPort->pstFirst = pstShip;
        } else {
            pstPort->pstLast->pstNext = pstShip;
        }
        pstPort->pstLast = pstShip;
        pstShip->eState = PORT_QUEUED;
        nSent = sendTextFrame(pstShip->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "WAIT");
    }
    pthread_mutex_unlock(&pstPort->stMutex);

    if (nSent != 0) {
        return -1;
    }
    return nDocked;
}

/***********************************************
 *
 * @Name: portLeave
 * @Def: A ship leaves the port, whatever its state. A waiting ship is taken
 *       out of the list. A docked ship hands its dock to the oldest waiting
 *       ship and sends it DOCKED; if nobody is waiting the dock becomes free.
 * @Arg: In/Out: pstPort pointer to the port
 *       In/Out: pstShip port data of the ship that leaves
 *       Out: ppsNextName copy of the name of the ship that received the dock
 *            (the caller frees it), or NULL if no ship received it
 * @Ret: The state the ship had before leaving.
 *
 ***********************************************/
tPortState portLeave(tPort *pstPort, tPortShip *pstShip, char **ppsNextName) {
    tPortState eBefore;
    tPortShip *pstNext;

    *ppsNextName = NULL;

    pthread_mutex_lock(&pstPort->stMutex);
    eBefore = pstShip->eState;
    if (eBefore == PORT_QUEUED) {
        removeWaiting(pstPort, pstShip);
    } else if (eBefore == PORT_DOCKED) {
        // The dock goes to the first waiting ship that can still be reached.
        // A ship whose socket is broken is dropped and the next one is tried
        pstNext = takeFirst(pstPort);
        while (pstNext != NULL &&
               sendTextFrame(pstNext->nFd, TYPE_ISLAND_ARRIVE, FLAGS_OK, "DOCKED") != 0) {
            pstNext->eState = PORT_GONE;
            pstNext = takeFirst(pstPort);
        }
        if (pstNext != NULL) {
            // Hand-off: the dock stays counted, it only changes owner
            pstNext->eState = PORT_DOCKED;
            *ppsNextName = dupString(pstNext->psName);
        } else {
            pstPort->nDocked--;
        }
    }
    pstShip->eState = PORT_GONE;
    pthread_mutex_unlock(&pstPort->stMutex);
    return eBefore;
}

/***********************************************
 *
 * @Name: portDestroy
 * @Def: Destroys the mutex of the port. Every ship must have left before.
 * @Arg: In/Out: pstPort pointer to the port
 * @Ret: None.
 *
 ***********************************************/
void portDestroy(tPort *pstPort) {
    pstPort->pstFirst = NULL;
    pstPort->pstLast = NULL;
    pthread_mutex_destroy(&pstPort->stMutex);
}