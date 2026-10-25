// voyage_service.h
#include <pthread.h>
#include "protocol.h"
#include "voyage_config.h"

//TODO: idk if we need these structs

typedef struct {
    tVoyage *pstVoyages;
    int nNumVoyages;
    pthread_mutex_t stMutex;    // protects the voyage list
} tVoyageStore;

typedef struct {
    int nFd;                    // this client's socket
    char sName[100];            // empty until CONNECT succeeds
    int nIdentified;            // 0 until CONNECT succeeds
    tVoyageStore *pstStore;     // shared between all client threads
} tClientSession;

void serveClient(tClientSession *pstSession);