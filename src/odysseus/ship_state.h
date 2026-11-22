/***********************************************
 *
 * @File : ship_state.h
 * @Purpose : Saving and restoring the progress of an Odysseus between executions
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 22/11/26
 *
 ***********************************************/
#ifndef CODE_SHIP_STATE_H
#define CODE_SHIP_STATE_H

// System Includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

// Project Includes
#include "ship_config.h"

// Results of loadShipState
#define STATE_NONE 0            // no saved state: the ship starts from its config
#define STATE_RESTORED 1        // the saved state was restored
#define STATE_VOYAGE_LOST 2     // restored, but the files of the voyage were missing: voyage dropped
#define STATE_DAMAGED -1        // the file could not be read: the ship starts from its config

//Prototypes
int saveShipState(const tShip *pstShip);
int loadShipState(tShip *pstShip);
void deleteShipState(const tShip *pstShip);

#endif