/***********************************************
 *
 * @File : socket_utils.c
 * @Purpose : TCP socket helpers shared by Odysseus, Ithaca and Island:
 *            opening a listening socket for the servers, accepting clients,
 *            connecting to a server and closing sockets safely. Addresses
 *            always come from the configuration files, never from localhost.
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 1/11/26
 *
 ***********************************************/

#include "socket_utils.h"

/***********************************************
 *
 * @Name: createServerSocket
 * @Def: Creates a TCP socket bound to the given IP and port and starts
 *       listening, so the server can accept clients.
 * @Arg: In: psIp IPv4 address
 *       In: nPort port to listen on
 * @Ret: Listening socket, or -1 on error with errno set to the cause.
 *
 ***********************************************/
int createServerSocket(const char *psIp, int nPort) {
    struct sockaddr_in stAddr;
    int nOpt = 1;
    int nFd = socket(AF_INET, SOCK_STREAM, 0);

    if (nFd < 0) {
        return -1;
    }

    // Allows restarting right after CTRL+C without "Address already in use"
    if (setsockopt(nFd, SOL_SOCKET, SO_REUSEADDR, &nOpt, sizeof(nOpt)) < 0) {
        int nErr = errno;
        close(nFd);
        errno = nErr;
        return -1;
    }

    memset(&stAddr, 0, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons((unsigned short)nPort);
    if (inet_pton(AF_INET, psIp, &stAddr.sin_addr) != 1) {
        close(nFd);
        return -1;
    }

    if (bind(nFd, (struct sockaddr *)&stAddr, sizeof(stAddr)) < 0 ||
        listen(nFd, LISTEN_BACKLOG) < 0) {
        int nErr = errno;
        close(nFd);
        errno = nErr;
        return -1;
    }
    return nFd;
}

/***********************************************
 *
 * @Name: acceptClient
 * @Def: Blocks until a client connects to the listening socket.
 * @Arg: In: nServerFd socket returned by createServerSocket
 * @Ret: Socket connected to the new client, or -1 on error.
 *
 ***********************************************/
int acceptClient(int nServerFd) {
    return accept(nServerFd, NULL, NULL);
}

/***********************************************
 *
 * @Name: connectToServer
 * @Def: Opens a TCP connection to a server. Used by Odysseus to reach Ithaca
 *       and the islands.
 * @Arg: In: psIp IPv4 address
 *       In: nPort server port
 * @Ret: Connected socket, or -1 on error with errno set to the cause.
 *
 ***********************************************/
int connectToServer(const char *psIp, int nPort) {
    struct sockaddr_in stAddr;
    int nFd = socket(AF_INET, SOCK_STREAM, 0);

    if (nFd < 0) {
        return -1;
    }

    memset(&stAddr, 0, sizeof(stAddr));
    stAddr.sin_family = AF_INET;
    stAddr.sin_port = htons((unsigned short)nPort);
    if (inet_pton(AF_INET, psIp, &stAddr.sin_addr) != 1) {
        close(nFd);
        return -1;
    }

    if (connect(nFd, (struct sockaddr *)&stAddr, sizeof(stAddr)) < 0) {
        int nErr = errno;
        close(nFd);
        errno = nErr;
        return -1;
    }
    return nFd;
}

/***********************************************
 *
 * @Name: closeSocket
 * @Def: Closes a socket if it is open and sets it to -1. The -1 marks the
 *       socket as closed everywhere it is checked ("fd >= 0" means
 *       connected) and makes a second call harmless.
 * @Arg: In/Out: pnFd socket to close
 * @Ret: None.
 *
 ***********************************************/
void closeSocket(int *pnFd) {
    if (*pnFd >= 0) {
        close(*pnFd);
        *pnFd = -1;
    }
}

