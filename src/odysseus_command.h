/***********************************************
 *
 * @File : odysseus_command.h
 * @Purpose : Header file for command handling in the Odysseus application
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CODE_ODYSSEUS_COMMAND_H
#define CODE_ODYSSEUS_COMMAND_H

#define MAX_ARGS 4

typedef int (*CommandHandler)(int argc, char *argv[]);  

typedef enum {
    ARG_NONE,
    ARG_NUMERIC,
    ARG_TEXT
} ArgKind;

typedef struct {
    const char     *word1;      
    const char     *word2;      
    int             numArgs;
    ArgKind         argKinds[2]; 
    const char     *usage;
    CommandHandler  handler;
} Command;

void processLine(char *tokens[], int tokenCount);
int tokenizeLine(char *line, char *argv[], int maxTokens);

#endif //CODE_ODYSSEUS_COMMAND_H