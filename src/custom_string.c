#include "custom_string.h"

char *readLineDynamic(int fd) {
    int cap = 16;
    int len = 0;
    char *buf = malloc(cap);
    if (buf == NULL) {
        return NULL;
    }

    while (1) {
        char c;
        int n = read(fd, &c, 1);

        if (n <= 0) {          //EOF or error
            if (len == 0) {
                free(buf);
                return NULL;
            }
            break;
        }
        if (c == '\n') {
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