/***********************************************
 *
 * @File : odysseus_parser.c
 * @Purpose : Parser for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "odysseus_parser.h"

/***********************************************
 *
 * @Name: parseOdysseus
 * @Def: Parses the Odysseus configuration file and populates a tOdysseus structure.
 * @Arg: In: psConfigPath path to the configuration file
 * @Ret: Returns populated tOdysseus structure.
 *
 ***********************************************/
tOdysseus parseOdysseus(char * psConfigPath) {
    int nFd = 0;
    char *psBuf = NULL;
    tOdysseus stOd;
    
    memset(&stOd, 0, sizeof(stOd)); //like malloc and all pointers NULL

    nFd = open(psConfigPath, O_RDONLY);
    if (nFd < 0) {
        write(STDERR_FILENO, "Error: failed opening config file\n", 34);
        exit(EXIT_FAILURE);
    }

    //Read whole file into buffer
    psBuf = readDynamic(nFd,0);
    close(nFd);
    if (!psBuf) {
        write(STDERR_FILENO, "Error: malloc failed opening config file\n", 40);
        exit(EXIT_FAILURE);
    }

    stOd.psName = dupString(strtok(psBuf, " \n"));
    stOd.psFilePath = dupString(strtok(NULL, " \n"));
    stOd.psIthacaIp = dupString(strtok(NULL, " \n"));
    stOd.nIthacaPort = atoi(strtok(NULL, " \n"));

    stOd.psIslandName = dupString(strtok(NULL, " \n"));
    stOd.psIslandIp = dupString(strtok(NULL, " \n"));
    stOd.nIslandPort = atoi(strtok(NULL, " \n"));

    stOd.nInitialMoney = atoi(strtok(NULL, " \n"));
    stOd.nNumFoodItems = atoi(strtok(NULL, " \n"));

    stOd.pstFoodSupplies = malloc(sizeof(tFood) * stOd.nNumFoodItems);
    if (!stOd.pstFoodSupplies && stOd.nNumFoodItems > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", 21);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < stOd.nNumFoodItems; i++) {
        stOd.pstFoodSupplies[i].psProduct  = dupString(strtok(NULL, " \n"));
        stOd.pstFoodSupplies[i].nQuantity = atoi(strtok(NULL, " \n"));
    }

    free(psBuf);   //free after copies made
    return stOd;

}

/***********************************************
 *
 * @Name: freeOdysseus
 * @Def: Frees all dynamically allocated memory inside a tOdysseus structure.
 * @Arg: In/Out: pstOd pointer to tOdysseus structure to free
 * @Ret: None.
 *
 ***********************************************/
void freeOdysseus(tOdysseus *pstOd) {
    if (pstOd != NULL) {
        free(pstOd->psName);
        free(pstOd->psFilePath);
        free(pstOd->psIthacaIp);
        free(pstOd->psIslandName);
        free(pstOd->psIslandIp);

        if (pstOd->pstFoodSupplies != NULL) {
            for (int i = 0; i < pstOd->nNumFoodItems; i++) {
                free(pstOd->pstFoodSupplies[i].psProduct);
            }
            free(pstOd->pstFoodSupplies);
        }

        // Reset memory to prevent dangling pointers
        memset(pstOd, 0, sizeof(*pstOd));
    }
}