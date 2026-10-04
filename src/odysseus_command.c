/***********************************************
 *
 * @File : odysseus_command.c
 * @Purpose : Command handling for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "odysseus_command.h"

/***********************************************
 *
 * @Name: cmdDef
 * @Def: Default dummy command handler function.
 * @Arg: In: argc count of command arguments
 *       In: argv array of argument strings
 * @Ret: Returns 0.
 *
 ***********************************************/
//Will be replaced with actual functions for each command.
static int cmd_def(int argc, char *argv[]){
    (void) argc;
    (void) argv;
    return 0;
}

static tCommand g_astCommands[] = {
    { "CONNECT", "ITHACA", 0, { 0 }, "Usage: CONNECT ITHACA\n", cmd_def },
    { "LIST", "VOYAGES", 0, { 0 }, "Usage: LIST VOYAGES\n", cmd_def },
    { "ACCEPT", NULL, 1, { ARG_NUMERIC }, "Usage: ACCEPT <voyage_id>\n", cmd_def },
    { "SAIL", NULL, 1, { ARG_TEXT }, "Usage: SAIL <island>\n", cmd_def },
    { "MAP", NULL, 0, { 0 }, "Usage: MAP\n", cmd_def },
    { "LIST", "MARKET", 0, { 0 }, "Usage: LIST MARKET\n", cmd_def },
    { "BUY", NULL, 2, { ARG_TEXT, ARG_NUMERIC }, "Usage: BUY <product> <amount>\n", cmd_def },
    { "SELL", NULL, 2, { ARG_TEXT, ARG_NUMERIC  }, "Usage: SELL <product> <amount>\n", cmd_def },
    { "STATUS", NULL, 0, { 0 }, "Usage: STATUS\n", cmd_def },
    { "DELIVER", NULL, 0, { 0 }, "Usage: DELIVER\n", cmd_def },
    { "CLAIM", NULL, 0, { 0 }, "Usage: CLAIM\n", cmd_def },
};

static const int gnNumCommands = sizeof(g_astCommands) / sizeof(g_astCommands[0]);

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
static tCommand *findCommand(char *ppsTokens[], int nTokenCount, int *pnNameWords) {
    for (int i = 0; i < gnNumCommands; i++) {
        if (g_astCommands[i].psWord2 != NULL) {
            if (nTokenCount >= 2 &&
                strcasecmp(ppsTokens[0], g_astCommands[i].psWord1) == 0 &&
                strcasecmp(ppsTokens[1], g_astCommands[i].psWord2) == 0) {
                *pnNameWords = 2;
                return &g_astCommands[i];
            }
        }
    }
    for (int i = 0; i < gnNumCommands; i++) {
        if (g_astCommands[i].psWord2 == NULL) {
            if (nTokenCount >= 1 && strcasecmp(ppsTokens[0], g_astCommands[i].psWord1) == 0) {
                *pnNameWords = 1;
                return &g_astCommands[i];
            }
        }
    }
    return NULL;
}

/***********************************************
 *
 * @Name: processLine
 * @Def: Parses input tokens, validates arguments, and invokes target command handler.
 * @Arg: In: ppsTokens tokenized argument strings
 *       In: nTokenCount count of tokenized argument strings
 * @Ret: None.
 *
 ***********************************************/
void processLine(char *ppsTokens[], int nTokenCount) {
    int nNameWords = 0;
    int nGivenArgs = 0;
    char **ppsArgs = NULL;
    tCommand *pstCmd = NULL;
    
    if (nTokenCount == 0) {
        return;
    }

    pstCmd = findCommand(ppsTokens, nTokenCount, &nNameWords);

    if (pstCmd == NULL) {
        write(STDOUT_FILENO, "Unknown command\n", strlen("Unknown command\n"));
        return;
    }

    ppsArgs = &ppsTokens[nNameWords];
    nGivenArgs = nTokenCount - nNameWords;

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

    pstCmd->fHandler(nGivenArgs, ppsArgs);
    write(STDOUT_FILENO, "Command OK\n", strlen("Command OK\n"));
}