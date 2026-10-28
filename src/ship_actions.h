#ifndef CODE_SHIP_ACTIONS_H
#define CODE_SHIP_ACTIONS_H


#define RATION_KG 7

int cmdConnectIthaca(tShip *pstShip, int argc, char *argv[]);
int cmdListVoyages(tShip *pContext, int nArgc, char *ppsArgv[]);
int cmdAcceptVoyage(tShip *pstShip, int nArgc, char *ppsArgv[]);
int cmdSail(tShip *pstShip, int nArgc, char *ppsArgv[]);

#endif