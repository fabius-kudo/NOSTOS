/***********************************************
 *
 * @File : ithaca_parser.c
 * @Purpose : Parsing functions for Ithaca data
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "ithaca_parser.h"

/***********************************************
 *
 * @Name: parseIthaca
 * @Def: Parses the Ithaca configuration file and populates a tIthaca struct.
 * @Arg: In: psConfigPath path to configuration file
 * @Ret: Returns populated tIthaca structure.
 *
 ***********************************************/
tIthaca parseIthaca(char *psConfigPath) {
    int nFd = 0;
    char *psBuf = NULL;
    tIthaca stIthaca;
    
    memset(&stIthaca, 0, sizeof(stIthaca));

    nFd = open(psConfigPath, O_RDONLY);
    if (nFd < 0) {
        write(STDERR_FILENO, "Error: failed opening config file\n", strlen("Error: failed opening config file\n"));
        exit(EXIT_FAILURE);
    }

    psBuf = readDynamic(nFd, 0);
    close(nFd);
    if (!psBuf) {
        write(STDERR_FILENO, "Error: malloc failed opening config file\n", strlen("Error: malloc failed opening config file\n"));
        exit(EXIT_FAILURE);
    }

    stIthaca.psServerName = dupString(strtok(psBuf, " \n"));
    stIthaca.psPath       = dupString(strtok(NULL, " \n"));
    stIthaca.psIpAddress = dupString(strtok(NULL, " \n"));
    stIthaca.nPort       = atoi(strtok(NULL, " \n"));

    free(psBuf);
    return stIthaca;
}

/***********************************************
 *
 * @Name: freeIthaca
 * @Def: Frees dynamically allocated memory within a tIthaca struct.
 * @Arg: In/Out: pstIthaca pointer to tIthaca structure to free
 * @Ret: None.
 *
 ***********************************************/
void freeIthaca(tIthaca *pstIthaca) {
    if (NULL != pstIthaca) {
        free(pstIthaca->psServerName);
        free(pstIthaca->psPath);
        free(pstIthaca->psIpAddress);
        memset(pstIthaca, 0, sizeof(*pstIthaca));
    }
}

/***********************************************
 *
 * @Name: parseVoyages
 * @Def: Reads and parses voyage data from file into an array of tVoyage structures.
 * @Arg: In: psVoyagesPath path to voyages file
 *       Out: pnNumVoyages pointer to store the total voyage count
 * @Ret: Returns dynamically allocated array of tVoyage structures.
 *
 ***********************************************/
tVoyage *parseVoyages(char *psVoyagesPath, int *pnNumVoyages) {
    int nFd = 0;
    int nCount = 0;
    char *psBuf = NULL;
    char *psBufCopy = NULL;
    char *psLine = NULL;
    char *psTok = NULL;
    tVoyage *pstVoyages = NULL;

    nFd = open(psVoyagesPath, O_RDONLY);
    if (nFd < 0) {
        write(STDERR_FILENO, "Error: failed opening voyages file\n", strlen("Error: failed opening voyages file\n"));
        exit(EXIT_FAILURE);
    }

    psBuf = readDynamic(nFd, 0);
    close(nFd);
    if (!psBuf) {
        write(STDERR_FILENO, "Error: malloc failed opening voyages file\n", strlen("Error: malloc failed opening voyages file\n"));
        exit(EXIT_FAILURE);
    }

    // First pass: count lines, using a copy since strtok is destructive
    psBufCopy = dupString(psBuf);
    if (!psBufCopy) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    char *line = strtok(psBufCopy, "\n");
    while (line != NULL) {
        nCount++;
        line = strtok(NULL, "\n");
    }
    free(psBufCopy);

    pstVoyages = malloc(sizeof(tVoyage) * nCount);
    if (!pstVoyages && nCount > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    // Second pass: parse each line into a Voyage
    psTok = strtok(psBuf, " \n"); 
    for (int i = 0; i < nCount; i++) {
        pstVoyages[i].psObject      = dupString(psTok);
        pstVoyages[i].psFile        = dupString(strtok(NULL, " \n"));
        pstVoyages[i].psDestination = dupString(strtok(NULL, " \n"));
        pstVoyages[i].nReward       = atoi(strtok(NULL, " \n"));
        psTok = strtok(NULL, " \n");
    }

    free(psBuf);
    *pnNumVoyages = nCount;
    return pstVoyages;
}

/***********************************************
 *
 * @Name: freeVoyages
 * @Def: Frees array of tVoyage structures and internal allocated strings.
 * @Arg: In: pstVoyages array of tVoyage structures
 *       In: nNumVoyages total number of elements in array
 * @Ret: None.
 *
 ***********************************************/
void freeVoyages(tVoyage *pstVoyages, int nNumVoyages) {
    int i = 0;

    if (NULL != pstVoyages) {
        for (i = 0; i < nNumVoyages; i++) {
            free(pstVoyages[i].psObject);
            free(pstVoyages[i].psFile);
            free(pstVoyages[i].psDestination);
        }
        free(pstVoyages);
    }
}