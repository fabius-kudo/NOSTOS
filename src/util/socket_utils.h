#ifndef CODE_SOCKET_UTILS_H
#define CODE_SOCKET_UTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define LISTEN_BACKLOG 16

int createServerSocket(const char *psIp, int nPort);
int acceptClient(int nServerFd);
int connectToServer(const char *psIp, int nPort);
void closeSocket(int *pnFd);

#endif