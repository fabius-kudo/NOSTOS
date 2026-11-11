/***********************************************
 *
 * @File : socket_utils.h
 * @Purpose : Header file for TCP socket helpers shared by Odysseus, Ithaca and Island:
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 1/11/26
 *
 ***********************************************/

#ifndef CODE_SOCKET_UTILS_H
#define CODE_SOCKET_UTILS_H

// System Includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

// Constant Declarations
#define LISTEN_BACKLOG 16

int createServerSocket(const char *psIp, int nPort);
int acceptClient(int nServerFd);
int connectToServer(const char *psIp, int nPort);
void closeSocket(int *pnFd);

#endif