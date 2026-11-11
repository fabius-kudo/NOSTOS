/***********************************************
 *
 * @File : client_list.c
 * @Purpose : Keeps track of the client threads of a server, so they can be
 *            woken and joined before the server frees its shared data
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 11/11/26
 *
 ***********************************************/

#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>

#include "client_list.h"
#include "socket_utils.h"

/***********************************************
 *
 * @Name: clientListInit
 * @Def: Starts an empty list and its mutex.
 * @Arg: Out: pstList list to initialize
 * @Ret: None.
 *
 ***********************************************/
void clientListInit(tClientList *pstList) {
    pstList->pstHead = NULL;
    pthread_mutex_init(&pstList->stMutex, NULL);
}

/***********************************************
 *
 * @Name: clientListAdd
 * @Def: Adds a node for a new client at the start of the list. The thread
 *       is created afterwards by the caller, using the node as its argument.
 * @Arg: In/Out: pstList list of clients
 *       In: nFd socket of the client
 *       In: pvData data of the thread; the list frees it with the node
 * @Ret: The new node, or NULL if malloc fails (pvData is then not freed).
 *
 ***********************************************/
tClientNode *clientListAdd(tClientList *pstList, int nFd, void *pvData) {
    tClientNode *pstNode = malloc(sizeof(tClientNode));

    if (pstNode == NULL) {
        return NULL;
    }
    pstNode->nFd = nFd;
    pstNode->nDone = 0;
    pstNode->pvData = pvData;
    pstNode->pstList = pstList;

    pthread_mutex_lock(&pstList->stMutex);
    pstNode->pstNext = pstList->pstHead;
    pstList->pstHead = pstNode;
    pthread_mutex_unlock(&pstList->stMutex);
    return pstNode;
}

/***********************************************
 *
 * @Name: clientListDrop
 * @Def: Removes and frees a node (and its data) whose thread could not be
 *       created. The socket is not closed here.
 * @Arg: In/Out: pstList list of clients
 *       In: pstNode node to remove
 * @Ret: None.
 *
 ***********************************************/
void clientListDrop(tClientList *pstList, tClientNode *pstNode) {
    tClientNode **ppstLink;

    pthread_mutex_lock(&pstList->stMutex);
    ppstLink = &pstList->pstHead;
    while (*ppstLink != NULL && *ppstLink != pstNode) {
        ppstLink = &(*ppstLink)->pstNext;
    }
    if (*ppstLink != NULL) {
        *ppstLink = pstNode->pstNext;
    }
    pthread_mutex_unlock(&pstList->stMutex);

    free(pstNode->pvData);
    free(pstNode);
}

/***********************************************
 *
 * @Name: clientListMarkDone
 * @Def: Called by a client thread as its last action, so that the main
 *       thread knows it can be joined.
 * @Arg: In/Out: pstNode node of the calling thread
 * @Ret: None.
 *
 ***********************************************/
void clientListMarkDone(tClientNode *pstNode) {
    pthread_mutex_lock(&pstNode->pstList->stMutex);
    pstNode->nDone = 1;
    pthread_mutex_unlock(&pstNode->pstList->stMutex);
}

/***********************************************
 *
 * @Name: clientListWake
 * @Def: Shuts down the socket of every running client. A thread blocked
 *       reading that socket then reads "connection closed" and ends by itself.
 * @Arg: In/Out: pstList list of clients
 * @Ret: None.
 *
 ***********************************************/
void clientListWake(tClientList *pstList) {
    tClientNode *pstNode;

    pthread_mutex_lock(&pstList->stMutex);
    for (pstNode = pstList->pstHead; pstNode != NULL; pstNode = pstNode->pstNext) {
        if (!pstNode->nDone) {
            shutdown(pstNode->nFd, SHUT_RDWR);
        }
    }
    pthread_mutex_unlock(&pstList->stMutex);
}

/***********************************************
 *
 * @Name: clientListReap
 * @Def: Joins client threads, closes their sockets and frees their nodes
 *       and data.
 * @Arg: In/Out: pstList list of clients
 *       In: nAll 0 = only the threads that already finished,
 *                1 = all of them (waits for each one to end)
 * @Ret: None.
 *
 ***********************************************/
void clientListReap(tClientList *pstList, int nAll) {
    tClientNode *pstTaken = NULL;
    tClientNode **ppstLink;
    tClientNode *pstNode;

    // The nodes are taken out of the list with the mutex locked, but joined
    // with it unlocked: a thread that is ending needs the mutex to mark itself done
    pthread_mutex_lock(&pstList->stMutex);
    ppstLink = &pstList->pstHead;
    while (*ppstLink != NULL) {
        pstNode = *ppstLink;
        if (nAll || pstNode->nDone) {
            *ppstLink = pstNode->pstNext;
            pstNode->pstNext = pstTaken;
            pstTaken = pstNode;
        } else {
            ppstLink = &pstNode->pstNext;
        }
    }
    pthread_mutex_unlock(&pstList->stMutex);

    while (pstTaken != NULL) {
        pstNode = pstTaken;
        pstTaken = pstNode->pstNext;
        // After the join nobody else uses the socket or the data of the thread
        pthread_join(pstNode->tThreadId, NULL);
        closeSocket(&pstNode->nFd);
        free(pstNode->pvData);
        free(pstNode);
    }
}

/***********************************************
 *
 * @Name: clientListDestroy
 * @Def: Destroys the mutex of the list. All the clients must be reaped first.
 * @Arg: In/Out: pstList list of clients
 * @Ret: None.
 *
 ***********************************************/
void clientListDestroy(tClientList *pstList) {
    pthread_mutex_destroy(&pstList->stMutex);
}