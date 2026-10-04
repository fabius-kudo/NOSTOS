/***********************************************
 *
 * @File : custom_dynamic.c
 * @Purpose : Dynamic memory management functions
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "custom_dynamic.h"

/***********************************************
 *
 * @Name: readDynamic
 * @Def: Reads characters dynamically from a file descriptor into a buffer.
 * @Arg: In: nFd file descriptor to read from
 *       In: nUntilNewline flag indicating whether to stop at newline
 * @Ret: Returns pointer to dynamically allocated string, or NULL on failure.
 *
 ***********************************************/
char *readDynamic(int nFd, int nUntilNewline) {
    int nCap = 16;
    int nLen = 0;
    int nBytesRead = 0;
    char cChar = '\0';
    char *psBuf = (char *)malloc(nCap);
    char *psTmp = NULL;

    if (psBuf == NULL) {
        return NULL;
    }

    while (1) {
        nBytesRead = read(nFd, &cChar, 1);

        //error or interrupted by signal
        if (nBytesRead < 0) {
            free(psBuf);
            return NULL;
        }
        //EOF check
        if (nBytesRead == 0) {
            if (nLen == 0) {
                free(psBuf);
                return NULL; 
            }
            break;
        }

        if (cChar == '\n' && nUntilNewline) {
            break;
        }

        //for the \0
        if (nLen + 1 >= nCap) {
            nCap *= 2;
            psTmp = realloc(psBuf, nCap);
            if (psTmp == NULL) {
                free(psBuf);
                return NULL;
            }
            psBuf = psTmp;
        }
        psBuf[nLen++] = cChar;
    }

    psBuf[nLen] = '\0';
    return psBuf;
}

/***********************************************
 *
 * @Name: dupString
 * @Def: Creates a duplicate of a given NULL-terminated string.
 * @Arg: In: psSource pointer to the source string to duplicate
 * @Ret: Returns pointer to the duplicated string, or NULL on error.
 *
 ***********************************************/
char *dupString(const char *psSource) {
    char *psCopy = NULL;

    if (psSource == NULL) return NULL;

    // malloc includes /0
    psCopy = malloc(strlen(psSource) + 1);

    if (psCopy == NULL) {
        write(STDERR_FILENO, "Error: malloc failed in dupString\n", strlen("Error: malloc failed in dupString\n"));
        return NULL;                 
    }
    strcpy(psCopy, psSource);
    return psCopy;
}