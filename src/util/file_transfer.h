/***********************************************
 *
 * @File : file_transfer.h
 * @Purpose : Sending and receiving image files with the frame protocol
 *            (header 0x20, READY 0x22, data 0x21, final ACK 0x23)
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 21/11/26
 *
 ***********************************************/
#ifndef CODE_FILE_TRANSFER_H
#define CODE_FILE_TRANSFER_H

// System Includes
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

// Project Includes
#include "protocol.h"
#include "custom_io.h"

// Constant Declarations
#define MD5_LENGTH 32               // hexadecimal characters printed by md5sum

// Results of a transfer
#define TRANSFER_OK 0               // file sent/received and verified
#define TRANSFER_REJECTED 1         // the other side refused it or the check failed; the connection is still usable
#define TRANSFER_LOST -1            // the connection failed, got out of step or was stopped; it must be closed

//Prototypes
int computeMd5(const char *psPath, char *psMd5);
int prepareFile(const char *psPath, int *pnSize, char *psMd5);
const char *baseName(const char *psPath);
int sendFile(int nFd, const char *psKind, const char *psPath, int nSize, const char *psMd5, int nAsServer, const int *pnStop);
int receiveFile(int nFd, const char *psKind, const char *psDir, char **ppsPath, const int *pnStop);
int receiveFileFrom(int nFd, const tFrame *pstHeader, const char *psKind, const char *psDir, char **ppsPath, const int *pnStop);
int rejectFile(int nFd, const char *psReason);

#endif