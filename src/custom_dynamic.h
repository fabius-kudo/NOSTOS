/***********************************************
 *
 * @File : custom_dynamic.h
 * @Purpose : Header file for th e dynamic memory management functions
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#ifndef CUSTOM_DYNAMIC_H
#define CUSTOM_DYNAMIC_H

// custom_dynamic.h
char *readDynamic(int fd, int untilNewline);   // reads until '\n' or EOF
char *dupString(const char *s);  // heap copy of a string

#endif  //CUSTOM_DYNAMIC_H