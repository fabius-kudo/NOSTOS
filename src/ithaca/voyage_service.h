#ifndef CODE_VOYAGE_SERVICE_H
#define CODE_VOYAGE_SERVICE_H

#include <pthread.h>
#include "protocol.h"
#include "voyage_config.h"

typedef struct {
    tVoyage *pstVoyages;
    int nNumVoyages;
    pthread_mutex_t stMutex;
} tVoyageStore;

typedef struct {
    int nFd;
    char *psName;
    int nIdentified;
    tVoyageStore *pstStore;
} tClientSession;

void serveClient(tClientSession *pstSession);

#endif