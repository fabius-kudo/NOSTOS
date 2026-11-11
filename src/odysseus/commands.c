/***********************************************
 *
 * @File : commands.c
 * @Purpose : Command handling for the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "commands.h"

// Syntax shown when a command is written with wrong arguments
#define USAGE_CONNECT_ITHACA "Usage: CONNECT ITHACA\n"
#define USAGE_LIST_VOYAGES   "Usage: LIST VOYAGES\n"
#define USAGE_ACCEPT         "Usage: ACCEPT <voyage_id>\n"
#define USAGE_SAIL           "Usage: SAIL <island>\n"
#define USAGE_MAP            "Usage: MAP\n"
#define USAGE_LIST_MARKET    "Usage: LIST MARKET\n"
#define USAGE_BUY            "Usage: BUY <product> <amount>\n"
#define USAGE_BUY_MAP        "Usage: BUY MAP 1\n"
#define USAGE_SELL           "Usage: SELL <product> <amount>\n"
#define USAGE_STATUS         "Usage: STATUS\n"
#define USAGE_DELIVER        "Usage: DELIVER\n"
#define USAGE_CLAIM          "Usage: CLAIM\n"
#define MSG_UNKNOWN_COMMAND  "Unknown command\n"

/***********************************************
 *
 * @Name: isNumeric
 * @Def: Checks if a given string contains only numeric digits.
 * @Arg: In: psStr string to validate
 * @Ret: Returns 1 if numeric, 0 otherwise.
 *
 ***********************************************/
int isNumeric(const char *psStr) {
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
 *       Out: ppsArgv array to store tokenized string pointers
 *       In: nMaxTokens maximum allowed tokens
 * @Ret: Returns total number of extracted tokens.
 *
 ***********************************************/
int tokenizeLine(char *psLine, char *ppsArgv[], int nMaxTokens) {
    int nArgc = 0;
    char *psToken = strtok(psLine, " \t");

    while (psToken != NULL && nArgc < nMaxTokens) {
        ppsArgv[nArgc++] = psToken;
        psToken = strtok(NULL, " \t");
    }
    return nArgc;
}

/***********************************************
 *
 * @Name: isWord
 * @Def: Compares a token with a command word, ignoring upper/lower case.
 * @Arg: In: psToken word written by the user
 *       In: psWord command word
 * @Ret: Returns 1 if they are the same word, 0 otherwise.
 *
 ***********************************************/
int isWord(const char *psToken, const char *psWord) {
    return strcasecmp(psToken, psWord) == 0;
}

/***********************************************
 *
 * @Name: findCommand
 * @Def: Identifies the command written by the user. Two-word commands are
 *       checked first, so "BUY MAP" is not taken for a "BUY".
 * @Arg: In: ppsTokens array of token strings
 *       In: nTokenCount total number of input tokens
 *       Out: pnNameWords number of tokens that form the command name (1 or 2)
 * @Ret: Returns the identifier of the command, or CMD_UNKNOWN if none matches.
 *
 ***********************************************/
tCmdId findCommand(char *ppsTokens[], int nTokenCount, int *pnNameWords) {
    if (nTokenCount >= 2) {
        *pnNameWords = 2;
        if (isWord(ppsTokens[0], "CONNECT") && isWord(ppsTokens[1], "ITHACA")) {
            return CMD_CONNECT_ITHACA;
        }
        if (isWord(ppsTokens[0], "LIST") && isWord(ppsTokens[1], "VOYAGES")) {
            return CMD_LIST_VOYAGES;
        }
        if (isWord(ppsTokens[0], "LIST") && isWord(ppsTokens[1], "MARKET")) {
            return CMD_LIST_MARKET;
        }
        if (isWord(ppsTokens[0], "BUY") && isWord(ppsTokens[1], "MAP")) {
            return CMD_BUY_MAP;
        }
    }

    *pnNameWords = 1;
    if (isWord(ppsTokens[0], "ACCEPT")) {
        return CMD_ACCEPT;
    }
    if (isWord(ppsTokens[0], "SAIL")) {
        return CMD_SAIL;
    }
    if (isWord(ppsTokens[0], "MAP")) {
        return CMD_MAP;
    }
    if (isWord(ppsTokens[0], "BUY")) {
        return CMD_BUY;
    }
    if (isWord(ppsTokens[0], "SELL")) {
        return CMD_SELL;
    }
    if (isWord(ppsTokens[0], "STATUS")) {
        return CMD_STATUS;
    }
    if (isWord(ppsTokens[0], "DELIVER")) {
        return CMD_DELIVER;
    }
    if (isWord(ppsTokens[0], "CLAIM")) {
        return CMD_CLAIM;
    }
    return CMD_UNKNOWN;
}

/***********************************************
 *
 * @Name: argsMatch
 * @Def: Checks the number of arguments of a command and that the ones
 *       expected to be numbers are numbers.
 * @Arg: In: nArgc number of arguments given
 *       In: ppsArgs arguments, without the command name
 *       In: nExpected number of arguments the command needs (0, 1 or 2)
 *       In: eKind1 kind of the first argument
 *       In: eKind2 kind of the second argument
 * @Ret: Returns 1 if the arguments are valid, 0 otherwise.
 *
 ***********************************************/
int argsMatch(int nArgc, char *ppsArgs[], int nExpected, tArgKind eKind1, tArgKind eKind2) {
    if (nArgc != nExpected) {
        return 0;
    }
    if (nExpected >= 1 && eKind1 == ARG_NUMERIC && !isNumeric(ppsArgs[0])) {
        return 0;
    }
    if (nExpected >= 2 && eKind2 == ARG_NUMERIC && !isNumeric(ppsArgs[1])) {
        return 0;
    }
    return 1;
}

/***********************************************
 *
 * @Name: checkArguments
 * @Def: Validates the arguments of a command. If they are wrong it prints
 *       the expected syntax.
 * @Arg: In: eId identifier of the command
 *       In: nArgc number of arguments given
 *       In: ppsArgs arguments, without the command name
 * @Ret: Returns 1 if the command can be executed, 0 otherwise.
 *
 ***********************************************/
int checkArguments(tCmdId eId, int nArgc, char *ppsArgs[]) {
    int nOk = 0;

    switch (eId) {
        case CMD_CONNECT_ITHACA:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_CONNECT_ITHACA);
            }
            break;
        case CMD_LIST_VOYAGES:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_LIST_VOYAGES);
            }
            break;
        case CMD_ACCEPT:
            nOk = argsMatch(nArgc, ppsArgs, 1, ARG_NUMERIC, ARG_NONE);
            if (!nOk) {
                printF(USAGE_ACCEPT);
            }
            break;
        case CMD_SAIL:
            nOk = argsMatch(nArgc, ppsArgs, 1, ARG_TEXT, ARG_NONE);
            if (!nOk) {
                printF(USAGE_SAIL);
            }
            break;
        case CMD_MAP:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_MAP);
            }
            break;
        case CMD_LIST_MARKET:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_LIST_MARKET);
            }
            break;
        case CMD_BUY:
            nOk = argsMatch(nArgc, ppsArgs, 2, ARG_TEXT, ARG_NUMERIC);
            if (!nOk) {
                printF(USAGE_BUY);
            }
            break;
        case CMD_BUY_MAP:
            nOk = argsMatch(nArgc, ppsArgs, 1, ARG_NUMERIC, ARG_NONE);
            if (!nOk) {
                printF(USAGE_BUY_MAP);
            }
            break;
        case CMD_SELL:
            nOk = argsMatch(nArgc, ppsArgs, 2, ARG_TEXT, ARG_NUMERIC);
            if (!nOk) {
                printF(USAGE_SELL);
            }
            break;
        case CMD_STATUS:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_STATUS);
            }
            break;
        case CMD_DELIVER:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_DELIVER);
            }
            break;
        case CMD_CLAIM:
            nOk = argsMatch(nArgc, ppsArgs, 0, ARG_NONE, ARG_NONE);
            if (!nOk) {
                printF(USAGE_CLAIM);
            }
            break;
        case CMD_UNKNOWN:
            break;
    }
    return nOk;
}

/***********************************************
 *
 * @Name: executeCommand
 * @Def: Calls the handler of an already validated command. Each handler
 *       checks whether the command makes sense in the ship's current state
 *       and prints its own output.
 * @Arg: In/Out: pstShip ship state the command acts on
 *       In: eId identifier of the command to run
 *       In: nArgc number of arguments
 *       In: ppsArgs arguments, without the command name
 * @Ret: None.
 *
 ***********************************************/
void executeCommand(tShip *pstShip, tCmdId eId, int nArgc, char *ppsArgs[]) {
    switch (eId) {
        case CMD_CONNECT_ITHACA: cmdConnectIthaca(pstShip, nArgc, ppsArgs); break;
        case CMD_LIST_VOYAGES: cmdListVoyages(pstShip, nArgc, ppsArgs); break;
        case CMD_ACCEPT: cmdAcceptVoyage(pstShip, nArgc, ppsArgs); break;
        case CMD_SAIL: cmdSail(pstShip, nArgc, ppsArgs); break;
        case CMD_MAP: cmdMap(pstShip, nArgc, ppsArgs); break;
        case CMD_BUY_MAP: cmdBuyMap(pstShip, nArgc, ppsArgs); break;
        case CMD_LIST_MARKET: cmdListMarket(pstShip, nArgc, ppsArgs); break;
        case CMD_BUY: cmdBuy(pstShip, nArgc, ppsArgs); break;
        case CMD_SELL: cmdSell(pstShip, nArgc, ppsArgs); break;
        case CMD_STATUS: cmdStatus(pstShip, nArgc, ppsArgs); break;
        case CMD_DELIVER:
        case CMD_CLAIM:
        case CMD_UNKNOWN:
            break;                 // not implemented yet
    }
}

/***********************************************
 *
 * @Name: processLine
 * @Def: Parses input tokens, validates arguments, and invokes target command handler.
 * @Arg: In/Out: pstShip ship state the command acts on
 *       In: ppsTokens tokenized argument strings
 *       In: nTokenCount count of tokenized argument strings
 * @Ret: None.
 *
 ***********************************************/
void processLine(tShip *pstShip, char *ppsTokens[], int nTokenCount) {
    int nNameWords = 0;
    int nGivenArgs;
    char **ppsArgs;
    tCmdId eId;

    if (nTokenCount == 0) {
        return;
    }

    eId = findCommand(ppsTokens, nTokenCount, &nNameWords);
    if (eId == CMD_UNKNOWN) {
        printF(MSG_UNKNOWN_COMMAND);
        return;
    }

    ppsArgs = &ppsTokens[nNameWords];
    nGivenArgs = nTokenCount - nNameWords;

    if (!checkArguments(eId, nGivenArgs, ppsArgs)) {
        return;
    }
    executeCommand(pstShip, eId, nGivenArgs, ppsArgs);
}