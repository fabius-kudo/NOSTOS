#include "socket_utils.h"
#include <errno.h>

/***********************************************
 * @Name: createServerSocket
 * @Def: Creates a TCP socket bound to the given IP and port, ready to accept.
 * @Ret: Listening fd, or -1 on error.
 ***********************************************/
//TODO: idk if we need this
int createServerSocket(const char *psIp, int nPort) {
    struct sockaddr_in stAddr;
    int nOpt = 1;
    int nFd = socket(AF_INET, SOCK_STREAM, 0);

    if (nFd < 0) {
        return -1;
    }

    // Allows restarting right after CTRL+C without "Address already in use"
    if (setsockopt(nFd, SOL_SOCKET, SO_REUSEADDR, &nOpt, sizeof(nOpt)) < 0) {
        close(nFd);
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
        close(nFd);
        return -1;
    }
    return nFd;
}

/***********************************************
 * @Name: acceptClient
 * @Def: Waits for a client connection.
 * @Ret: Client fd, or -1 on error (errno == EINTR if CTRL+C interrupted it).
 ***********************************************/
int acceptClient(int nServerFd) {
    return accept(nServerFd, NULL, NULL);
}

/***********************************************
 * @Name: connectToServer
 * @Def: Opens a TCP connection to the given IP and port.
 * @Ret: Connected fd, or -1 on error.
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
 * @Name: closeSocket
 * @Def: Closes a socket and sets it to -1 so it can't be closed twice.
 ***********************************************/
void closeSocket(int *pnFd) {
    if (*pnFd >= 0) {
        close(*pnFd);
        *pnFd = -1;
    }
}