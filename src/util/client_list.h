/***********************************************
 *
 * @File : client_list.h
 * @Purpose : Keeps track of the client threads of a server, so they can be
 *            woken and joined before the server frees its shared data
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 11/11/26
 *
 ***********************************************/

#ifndef CLIENT_LIST_H
#define CLIENT_LIST_H

#include <pthread.h>

typedef struct tClientNode {
    pthread_t tThreadId;
    int nFd;                        // closed by clientListReap, after the join
    int nDone;                      // set by the thread when it ends
    void *pvData;                   // data of the thread (its session), freed with the node
    struct tClientList *pstList;    // so the thread can mark itself done
    struct tClientNode *pstNext;
} tClientNode;

typedef struct tClientList {
    pthread_mutex_t stMutex;        // protects the list and every nDone flag
    tClientNode *pstHead;
} tClientList;

void clientListInit(tClientList *pstList);
tClientNode *clientListAdd(tClientList *pstList, int nFd, void *pvData);
void clientListDrop(tClientList *pstList, tClientNode *pstNode);
void clientListMarkDone(tClientNode *pstNode);
void clientListWake(tClientList *pstList);
void clientListReap(tClientList *pstList, int nAll);
void clientListDestroy(tClientList *pstList);

#endif