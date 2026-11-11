/***********************************************
 *
 * @File : voyage_service.h
 * @Purpose : Header file for the server-side handling of the Odysseus <-> Ithaca protocol.
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 1/11/26
 *
 ***********************************************/

#ifndef CODE_VOYAGE_SERVICE_H
#define CODE_VOYAGE_SERVICE_H

// System Includes
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Project Includes
#include "protocol.h"
#include "voyage_config.h"

// Custom Type Definitions
typedef enum {
    ACCEPT_OK,
    ACCEPT_ALREADY_HAVE_VOYAGE,
    ACCEPT_NOT_FOUND,
    ACCEPT_NOT_AVAILABLE,
    ACCEPT_INTERNAL_ERROR
} tAcceptResult;
typedef struct {
    tVoyage *pstVoyages;
    int nNumVoyages;
    pthread_mutex_t stMutex;
} tVoyageStore;

//State of one connected Odysseus
typedef struct {
    int nFd;
    char *psName;
    int nIdentified;
    tVoyageStore *pstStore;
} tClientSession;

void serveClient(tClientSession *pstSession);

#endif