/***********************************************
 *
 * @File : commands.c
 * @Purpose : Command handling for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "commands.h"

/***********************************************
 *
 * @Name: isNumeric
 * @Def: Checks if a given string contains only numeric digits.
 * @Arg: In: psStr string to validate
 * @Ret: Returns 1 if numeric, 0 otherwise.
 *
 ***********************************************/
static int isNumeric(const char *psStr) {
    if (psStr == NULL || *psStr == '\0') {
        return 0;
    }
    for (int i = 0; psStr[i] != '\0'; i++) {
        if (psStr[i] < '0' || psStr[i] > '9') {
            return 0;
        }
    }
    return 1;
}

/***********************************************
 *
 * @Name: tokenizeLine
 * @Def: Tokenizes a input line string by spaces and tabs into an argument array.
 * @Arg: In: psLine raw input line string
 *       Out: argv array to store tokenized string pointers
 *       In: nMaxTokens maximum allowed tokens
 * @Ret: Returns total number of extracted tokens.
 *
 ***********************************************/
int tokenizeLine(char *psLine, char *argv[], int nMaxTokens) {
    int nArgc = 0;
    char *psToken = strtok(psLine, " \t");

    while (psToken != NULL && nArgc < nMaxTokens) {
        argv[nArgc++] = psToken;
        psToken = strtok(NULL, " \t");
    }
    return nArgc;
}

//changed
/***********************************************
 *
 * @Name: findCommand
 * @Def: Matches tokenized input words against available registered commands.
 * @Arg: In: ppsTokens array of token strings
 *       In: nTokenCount total number of input tokens
 *       Out: pnNameWords pointer to store matching command word count (1 or 2)
 * @Ret: Returns pointer to matched tCommand structure, or NULL if non-existent.
 *
 ***********************************************/
static const tCommand *findCommand(const tCommand *pstCmds, int nNumCmds,
                                   char *ppsTokens[], int nTokenCount, int *pnNameWords) {
    for (int i = 0; i < nNumCmds; i++) {
        if (pstCmds[i].psWord2 != NULL && nTokenCount >= 2 &&
            strcasecmp(ppsTokens[0], pstCmds[i].psWord1) == 0 &&
            strcasecmp(ppsTokens[1], pstCmds[i].psWord2) == 0) {
            *pnNameWords = 2;
            return &pstCmds[i];
        }
    }
    for (int i = 0; i < nNumCmds; i++) {
        if (pstCmds[i].psWord2 == NULL && nTokenCount >= 1 &&
            strcasecmp(ppsTokens[0], pstCmds[i].psWord1) == 0) {
            *pnNameWords = 1;
            return &pstCmds[i];
        }
    }
    return NULL;
}


static void executeCommand(tShip *pstShip, tCmdId id, int nArgc, char *ppsArgs[]) {
    switch (id) {
        case CMD_CONNECT_ITHACA: cmdConnectIthaca(pstShip, nArgc, ppsArgs); break;
        case CMD_LIST_VOYAGES: cmdListVoyages(pstShip, nArgc, ppsArgs); break;
        case CMD_ACCEPT: cmdAcceptVoyage(pstShip, nArgc, ppsArgs); break;
        case CMD_SAIL: cmdSail(pstShip, nArgc, ppsArgs); break;
        case CMD_MAP:
        case CMD_LIST_MARKET:
        case CMD_BUY:
        case CMD_SELL:
        case CMD_STATUS:
        case CMD_DELIVER:
        case CMD_CLAIM:
            break;                 // not implemented yet
    }
}

//changed
/***********************************************
 *
 * @Name: processLine
 * @Def: Parses input tokens, validates arguments, and invokes target command handler.
 * @Arg: In: ppsTokens tokenized argument strings
 *       In: nTokenCount count of tokenized argument strings
 * @Ret: None.
 *
 ***********************************************/
void processLine(tShip *pstShip, char *ppsTokens[], int nTokenCount) {
    const tCommand astCommands[] = {
        { "CONNECT", "ITHACA",  0, { 0 }, "Usage: CONNECT ITHACA\n", CMD_CONNECT_ITHACA },
        { "LIST", "VOYAGES", 0, { 0 }, "Usage: LIST VOYAGES\n", CMD_LIST_VOYAGES },
        { "ACCEPT",  NULL, 1, { ARG_NUMERIC }, "Usage: ACCEPT <voyage_id>\n", CMD_ACCEPT },
        { "SAIL", NULL, 1, { ARG_TEXT }, "Usage: SAIL <island>\n", CMD_SAIL },
        { "MAP", NULL, 0, { 0 }, "Usage: MAP\n", CMD_MAP },
        { "LIST", "MARKET", 0, { 0 }, "Usage: LIST MARKET\n", CMD_LIST_MARKET },
        { "BUY", NULL, 2, { ARG_TEXT, ARG_NUMERIC }, "Usage: BUY <product> <amount>\n", CMD_BUY },
        { "SELL", NULL, 2, { ARG_TEXT, ARG_NUMERIC }, "Usage: SELL <product> <amount>\n", CMD_SELL },
        { "STATUS", NULL, 0, { 0 }, "Usage: STATUS\n", CMD_STATUS },
        { "DELIVER", NULL, 0, { 0 }, "Usage: DELIVER\n", CMD_DELIVER },
        { "CLAIM", NULL, 0, { 0 }, "Usage: CLAIM\n", CMD_CLAIM },
    };
    const int nNumCmds = sizeof(astCommands) / sizeof(astCommands[0]);   

    if (nTokenCount == 0) {
        return;
    }

    int nNameWords = 0;
    const tCommand *pstCmd = findCommand(astCommands, nNumCmds, ppsTokens, nTokenCount, &nNameWords);
    if (pstCmd == NULL) {
        write(STDOUT_FILENO, "Unknown command\n", strlen("Unknown command\n"));
        return;
    }

    char **ppsArgs = &ppsTokens[nNameWords];
    int nGivenArgs = nTokenCount - nNameWords;

    if (nGivenArgs != pstCmd->nNumArgs) {
        write(STDOUT_FILENO, pstCmd->psUsage, strlen(pstCmd->psUsage));
        return;
    }
    for (int i = 0; i < nGivenArgs; i++) {
        if (pstCmd->aArgKinds[i] == ARG_NUMERIC && !isNumeric(ppsArgs[i])) {
            write(STDOUT_FILENO, pstCmd->psUsage, strlen(pstCmd->psUsage));
            return;
        }
    }

    executeCommand(pstShip, pstCmd->id, nGivenArgs, ppsArgs);
}