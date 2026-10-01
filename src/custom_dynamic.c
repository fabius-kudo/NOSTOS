#include "custom_dynamic.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

char *readDynamic(int fd, int untilNewline) {
    int cap = 16;
    int len = 0;
    char *buf = malloc(cap);
    if (buf == NULL) {
        return NULL;
    }

    while (1) {
        char c;
        int n = read(fd, &c, 1);

        if (n < 0) {                //error or interrupted by signal
            free(buf);
            return NULL;
        }
        if (n == 0) {               //EOF
            if (len == 0) {
                free(buf);
                return NULL; 
            }
            break;
        }
        if (c == '\n' && untilNewline) {
            break;
        }

        if (len + 1 >= cap) {  //for the \0
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (tmp == NULL) {
                free(buf);
                return NULL;
            }
            buf = tmp;
        }
        buf[len++] = c;
    }

    buf[len] = '\0';
    return buf;
}


char *dupString(const char *s) {
    char *copy = malloc(strlen(s) + 1); // \0
    if (copy == NULL) {
        write(STDERR_FILENO, "Error: malloc failed in dupString\n", 35);
        return NULL;                 
    }
    strcpy(copy, s);
    return copy;
}