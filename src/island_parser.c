/***********************************************
 *
 * @File : island_parser.c
 * @Purpose : Parsing functions for island data
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/
#include "island_parser.h"


/***********************************************
 *
 * @Name: parseIsland
 * @Def: Parses an island configuration file and populates a tIsland struct.
 * @Arg: In: psIslandsPath path to the island configuration file
 * @Ret: Returns populated tIsland structure.
 *
 ***********************************************/
tIsland parseIsland(char *psIslandsPath) {
    int nFd = 0;
    int nCount = 0;
    char *psBuf = NULL;
    char *psBufCopy = NULL;
    char *psLine = NULL;
    tIsland stIsland;
    memset(&stIsland, 0, sizeof(stIsland));

    nFd = open(psIslandsPath, O_RDONLY);
    if (0 > nFd) {
        write(STDERR_FILENO, "Error: failed to open island file\n", strlen("Error: failed to open island file\n"));
        exit(EXIT_FAILURE);
    }

    psBuf = readDynamic(nFd, 0);
    close(nFd);
    if (NULL == psBuf) {
        write(STDERR_FILENO, "Error: failed reading island file\n", strlen("Error: failed reading island file\n"));
        exit(EXIT_FAILURE);
    }

    psBufCopy = dupString(psBuf);
    if (NULL == psBufCopy) {
        free(psBuf);
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    // Counting lines after marker
    psLine = strtok(psBufCopy, "\n");
    while (psLine != NULL && strcmp(psLine, "--- ROUTES ---") != 0) {
        psLine = strtok(NULL, "\n");
    }
    if (psLine != NULL) {                    
        psLine = strtok(NULL, "\n");
        while (psLine != NULL) {
            nCount++;
            psLine = strtok(NULL, "\n");
        }
    }
    free(psBufCopy);

    // Real pass
    stIsland.psName = dupString(strtok(psBuf, "\n"));
    stIsland.psPath = dupString(strtok(NULL, "\n"));
    stIsland.psIpAddress = dupString(strtok(NULL, " \n"));
    stIsland.nPort = atoi(strtok(NULL, " \n"));
    stIsland.nMaxCapacity = atoi(strtok(NULL, " \n"));
    strtok(NULL, "\n");                  // consume the "--- ROUTES ---" line

    stIsland.pstRoutes = malloc(sizeof(tRoute) * nCount);
    if (!stIsland.pstRoutes && nCount > 0) {
        write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nCount; i++) {
        stIsland.pstRoutes[i].psName       = dupString(strtok(NULL, " \n"));
        stIsland.pstRoutes[i].psIpAddress = dupString(strtok(NULL, " \n"));
        stIsland.pstRoutes[i].nPort       = atoi(strtok(NULL, " \n"));
    }
    stIsland.nNumRoutes = nCount;

    free(psBuf);
    return stIsland;
}

/***********************************************
 *
 * @Name: freeIsland
 * @Def: Frees dynamically allocated memory in a tIsland struct.
 * @Arg: In: stIsland tIsland structure to free
 * @Ret: None.
 *
 ***********************************************/
void freeIsland(tIsland stIsland) {
    free(stIsland.psName);
    free(stIsland.psPath);
    free(stIsland.psIpAddress);
    for (int i = 0; i < stIsland.nNumRoutes; i++) {
        free(stIsland.pstRoutes[i].psName);
        free(stIsland.pstRoutes[i].psIpAddress);
    }
    free(stIsland.pstRoutes);
}

/***********************************************
 *
 * @Name: parseStock
 * @Def: Reads binary stock file into a dynamically allocated array of products.
 * @Arg: In: psStockPath path to stock data file
 *       Out: pnNumProducts pointer to store total product count
 * @Ret: Returns array of tProduct structs, or NULL/exits on error.
 *
 ***********************************************/
tProduct *parseStock(char *stock_path, int *num_products) {
    int nFd = 0;
    int nCount = 0;
    tProduct *pstProducts = NULL;
    tProduct *pstTmp = NULL;
    tProduct stTemp;

    nFd = open(stock_path, O_RDONLY);
    if (nFd < 0) {
        write(STDERR_FILENO, "Error: failed to open stock file\n", strlen("Error: failed to open stock file\n"));
        exit(EXIT_FAILURE);
    }

    
    while (read(nFd, &stTemp, sizeof(tProduct)) == (ssize_t)sizeof(tProduct)) {
        pstTmp = realloc(pstProducts, sizeof(tProduct) * (nCount + 1));
        if (!pstTmp) {
            free(pstProducts);
            close(nFd);
            write(STDERR_FILENO, "Error: malloc failed\n", strlen("Error: malloc failed\n"));
            exit(EXIT_FAILURE);
        }
        pstProducts = pstTmp;
        pstProducts[nCount++] = stTemp;
    }

    close(nFd);
    *num_products = nCount;
    return pstProducts;
}

/***********************************************
 *
 * @Name: filterRoutes
 * @Def: Filters valid island route configurations using the SPHRAGIS library.
 * @Arg: In/Out: pstIsland pointer to the island structure to filter
 * @Ret: Returns remaining route count on success, or -1 on failure.
 *
 ***********************************************/
int filterRoutes(tIsland *pstIsland) {
    int nRes = 0;
    int nK = 0;
    SPHRAGIS_Island stTmp;
    
    stTmp.name = pstIsland->psName;
    stTmp.known_island_count = pstIsland->nNumRoutes;
    stTmp.known_islands = NULL;

    if (pstIsland->nNumRoutes > 0) {
        stTmp.known_islands = calloc(pstIsland->nNumRoutes, sizeof(char *));
        if (!stTmp.known_islands) return -1;

        for (int i = 0; i < pstIsland->nNumRoutes; i++) {
            stTmp.known_islands[i] = dupString(pstIsland->pstRoutes[i].psName);
            if (!stTmp.known_islands[i]) {
                for (int j = 0; j < i; j++) free(stTmp.known_islands[j]);
                free(stTmp.known_islands);
                return -1;
            }
        }
    }

    nRes = SPHRAGIS_filter_island_configuration(&stTmp);
    if (nRes < 0) {                                   // invalid island or array/count
        for (int i = 0; i < pstIsland->nNumRoutes; i++) free(stTmp.known_islands[i]);
        free(stTmp.known_islands);
        return -1;
    }

    // Survivors keep their order, so a single index into them is enough
    int k = 0;
    for (int i = 0; i < pstIsland->nNumRoutes; i++) {
        if (k < nRes && strcmp(stTmp.known_islands[k], pstIsland->pstRoutes[i].psName) == 0) {
            pstIsland->pstRoutes[k++] = pstIsland->pstRoutes[i];   // compact in place
        } else {
            free(pstIsland->pstRoutes[i].psName);              // rejected route
            free(pstIsland->pstRoutes[i].psIpAddress);
        }
    }
    pstIsland->nNumRoutes = nRes;

    for (int i = 0; i < nRes; i++) free(stTmp.known_islands[i]);   // our remaining copies
    free(stTmp.known_islands);
    return nRes;
}