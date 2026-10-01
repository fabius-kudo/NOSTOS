#ifndef CUSTOM_DYNAMIC_H
#define CUSTOM_DYNAMIC_H

// custom_dynamic.h
char *readDynamic(int fd, int untilNewline);   // reads until '\n' or EOF
char *dupString(const char *s);  // heap copy of a string

#endif  //CUSTOM_DYNAMIC_H