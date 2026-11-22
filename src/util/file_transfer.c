/***********************************************
 *
 * @File : file_transfer.c
 * @Purpose : Sending and receiving image files with the frame protocol
 *            (header 0x20, READY 0x22, data 0x21, final ACK 0x23)
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 21/11/26
 *
 ***********************************************/

#define _GNU_SOURCE
#include <stdio.h>

#include "file_transfer.h"


/***********************************************
 *
 * @Name: computeMd5
 * @Def: Computes the MD5 of a file by running the system command md5sum in
 *       a child process. Its output is redirected to a pipe and the first 32
 *       characters (the hash) are read from it.
 * @Arg: In: psPath path of the file
 *       Out: psMd5 buffer of at least MD5_LENGTH + 1 bytes for the hash
 * @Ret: Returns 0 on success, -1 on error.
 *
 ***********************************************/
int computeMd5(const char *psPath, char *psMd5) {
    int anPipe[2];
    int nRead = 0;
    int nBytes;
    int nStatus = 0;
    pid_t nPid;

    if (pipe(anPipe) < 0) {
        return -1;
    }

    nPid = fork();
    if (nPid < 0) {
        close(anPipe[0]);
        close(anPipe[1]);
        return -1;
    }

    if (nPid == 0) {
        // Child: its standard output becomes the write end of the pipe, then
        // it is replaced by md5sum, which prints "<hash>  <file>"
        close(anPipe[0]);
        dup2(anPipe[1], STDOUT_FILENO);
        close(anPipe[1]);
        execlp("md5sum", "md5sum", psPath, (char *)NULL);
        // Only reached if md5sum could not be executed
        exit(EXIT_FAILURE);
    }

    // Parent: reads the hash and waits for the child, so it does not stay a zombie
    close(anPipe[1]);
    nBytes = read(anPipe[0], psMd5 + nRead, MD5_LENGTH - nRead);
    while (nBytes > 0 && nRead + nBytes < MD5_LENGTH) {
        nRead += nBytes;
        nBytes = read(anPipe[0], psMd5 + nRead, MD5_LENGTH - nRead);
    }
    if (nBytes > 0) {
        nRead += nBytes;
    }
    close(anPipe[0]);
    waitpid(nPid, &nStatus, 0);

    if (nRead != MD5_LENGTH || !WIFEXITED(nStatus) || WEXITSTATUS(nStatus) != 0) {
        return -1;
    }
    psMd5[MD5_LENGTH] = '\0';
    return 0;
}

/***********************************************
 *
 * @Name: getFileSize
 * @Def: Counts the bytes of a file by reading it whole.
 * @Arg: In: psPath path of the file
 * @Ret: Returns the size in bytes, or -1 if it cannot be read.
 *
 ***********************************************/
int getFileSize(const char *psPath) {
    unsigned char aucBuf[FRAME_DATA_SIZE];
    int nSize = 0;
    int nBytes;
    int nFd = open(psPath, O_RDONLY);

    if (nFd < 0) {
        return -1;
    }
    nBytes = read(nFd, aucBuf, sizeof(aucBuf));
    while (nBytes > 0) {
        nSize += nBytes;
        nBytes = read(nFd, aucBuf, sizeof(aucBuf));
    }
    close(nFd);
    if (nBytes < 0) {
        return -1;
    }
    return nSize;
}

/***********************************************
 *
 * @Name: prepareFile
 * @Def: Gets the size and the MD5 of a file before sending it. Done before
 *       any frame is sent, so a missing file can still be reported as a
 *       normal error instead of breaking the transfer halfway.
 * @Arg: In: psPath path of the file
 *       Out: pnSize size of the file in bytes
 *       Out: psMd5 buffer of at least MD5_LENGTH + 1 bytes for the hash
 * @Ret: Returns 0 if the file can be sent, -1 otherwise.
 *
 ***********************************************/
int prepareFile(const char *psPath, int *pnSize, char *psMd5) {
    *pnSize = getFileSize(psPath);
    if (*pnSize <= 0) {
        return -1;
    }
    return computeMd5(psPath, psMd5);
}

/***********************************************
 *
 * @Name: baseName
 * @Def: Returns the file name of a path, without its folders.
 * @Arg: In: psPath path of the file
 * @Ret: Pointer to the file name inside psPath.
 *
 ***********************************************/
const char *baseName(const char *psPath) {
    const char *psSlash = strrchr(psPath, '/');

    if (psSlash == NULL) {
        return psPath;
    }
    return psSlash + 1;
}

/***********************************************
 *
 * @Name: sendFile
 * @Def: Sends a file: header, waits for READY, sends the data frames and
 *       waits for the final ACK. The size and the MD5 come from prepareFile.
 * @Arg: In: nFd connected socket
 *       In: psKind transfer kind written in the header (OBJECT, DELIVERY, SIGNED, CLAIM)
 *       In: psPath path of the file to send
 *       In: nSize size of the file, from prepareFile
 *       In: psMd5 MD5 of the file, from prepareFile
 *       In: nAsServer 1 when Ithaca or an island sends, 0 when Odysseus sends
 * @Ret: TRANSFER_OK, TRANSFER_REJECTED or TRANSFER_LOST.
 *
 ***********************************************/
int sendFile(int nFd, const char *psKind, const char *psPath, int nSize, const char *psMd5, int nAsServer) {
    unsigned char ucHeaderFlags = nAsServer ? FLAG_RESPONSE : FLAG_REQUEST;
    unsigned char ucDataFlags = nAsServer ? (FLAG_RESPONSE | FLAG_BINARY) : FLAG_BINARY;
    unsigned char aucBlock[FRAME_DATA_SIZE];
    char sHeader[FRAME_DATA_SIZE + 1];
    int nRemaining = nSize;
    int nFileFd;
    int nLen;
    int nBlock;
    int nGot;
    tFrame stFrame;

    nLen = snprintf(sHeader, sizeof(sHeader), "%s&%s&%d&%s", psKind, baseName(psPath), nSize, psMd5);
    if (nLen < 0 || nLen > FRAME_DATA_SIZE) {
        return TRANSFER_REJECTED;
    }
    if (sendFrame(nFd, TYPE_FILE_HEADER, ucHeaderFlags, sHeader, nLen) != 0) {
        return TRANSFER_LOST;
    }

    // The receiver says if it can store the file
    if (recvFrame(nFd, &stFrame) != 1 || stFrame.ucType != TYPE_FILE_READY) {
        return TRANSFER_LOST;
    }
    if (stFrame.ucFlags != FLAGS_OK) {
        return TRANSFER_REJECTED;
    }

    nFileFd = open(psPath, O_RDONLY);
    while (nRemaining > 0) {
        nBlock = nRemaining < FRAME_DATA_SIZE ? nRemaining : FRAME_DATA_SIZE;

        // The receiver expects exactly the announced number of frames. If the
        // file can no longer be read, zeros are sent instead so both sides
        // stay in step; the receiver then reports an MD5_ERROR.
        nGot = 0;
        if (nFileFd >= 0) {
            nGot = read(nFileFd, aucBlock, nBlock);
        }
        if (nGot < nBlock) {
            memset(aucBlock + (nGot > 0 ? nGot : 0), 0, nBlock - (nGot > 0 ? nGot : 0));
        }

        if (sendFrame(nFd, TYPE_FILE_DATA, ucDataFlags, aucBlock, nBlock) != 0) {
            if (nFileFd >= 0) {
                close(nFileFd);
            }
            return TRANSFER_LOST;
        }
        nRemaining -= nBlock;
    }
    if (nFileFd >= 0) {
        close(nFileFd);
    }

    // The receiver checked the size and the MD5
    if (recvFrame(nFd, &stFrame) != 1 || stFrame.ucType != TYPE_FILE_ACK) {
        return TRANSFER_LOST;
    }
    if (stFrame.ucFlags != FLAGS_OK) {
        return TRANSFER_REJECTED;
    }
    return TRANSFER_OK;
}

/***********************************************
 *
 * @Name: isSafeName
 * @Def: Checks that a file name received from the network is a plain name,
 *       so it cannot write outside the folder of the process.
 * @Arg: In: psName file name from the header
 * @Ret: Returns 1 if the name can be used, 0 otherwise.
 *
 ***********************************************/
int isSafeName(const char *psName) {
    if (psName[0] == '\0' || strchr(psName, '/') != NULL) {
        return 0;
    }
    if (strcmp(psName, ".") == 0 || strcmp(psName, "..") == 0) {
        return 0;
    }
    return 1;
}

/***********************************************
 *
 * @Name: isMd5Text
 * @Def: Checks that a text is 32 hexadecimal characters.
 * @Arg: In: psText text to check
 * @Ret: Returns 1 if it is, 0 otherwise.
 *
 ***********************************************/
int isMd5Text(const char *psText) {
    int i;

    for (i = 0; psText[i] != '\0'; i++) {
        if (!((psText[i] >= '0' && psText[i] <= '9') || (psText[i] >= 'a' && psText[i] <= 'f') ||
              (psText[i] >= 'A' && psText[i] <= 'F'))) {
            return 0;
        }
    }
    return i == MD5_LENGTH;
}

/***********************************************
 *
 * @Name: receiveFile
 * @Def: Receives a file: reads the header, answers READY, stores the data
 *       frames in the given folder, checks size and MD5 and answers the
 *       final ACK. A file that fails the check is removed.
 * @Arg: In: nFd connected socket
 *       In: psKind transfer kind expected in the header
 *       In: psDir folder where the file is stored
 *       Out: ppsPath path of the stored file (the caller frees it), or NULL
 * @Ret: TRANSFER_OK, TRANSFER_REJECTED or TRANSFER_LOST.
 *
 ***********************************************/
int receiveFile(int nFd, const char *psKind, const char *psDir, char **ppsPath) {
    tFrame stHeader;

    *ppsPath = NULL;
    if (recvFrame(nFd, &stHeader) != 1) {
        return TRANSFER_LOST;
    }
    return receiveFileFrom(nFd, &stHeader, psKind, psDir, ppsPath);
}

/***********************************************
 *
 * @Name: rejectFile
 * @Def: Refuses a transfer the other side has already started: reads its
 *       header and answers READY with an error, so no data frames follow
 *       and the connection stays in step.
 * @Arg: In: nFd connected socket
 *       In: psReason reason sent in the READY frame
 * @Ret: TRANSFER_REJECTED, or TRANSFER_LOST if the header did not arrive.
 *
 ***********************************************/
int rejectFile(int nFd, const char *psReason) {
    tFrame stHeader;

    if (recvFrame(nFd, &stHeader) != 1 || stHeader.ucType != TYPE_FILE_HEADER) {
        return TRANSFER_LOST;
    }
    if (sendTextFrame(nFd, TYPE_FILE_READY, FLAGS_ERR, psReason) != 0) {
        return TRANSFER_LOST;
    }
    return TRANSFER_REJECTED;
}

/***********************************************
 *
 * @Name: receiveFileFrom
 * @Def: Same as receiveFile, for when the header frame has already been read
 *       (the caller had to look at it to know a file was coming).
 * @Arg: In: nFd connected socket
 *       In: pstHeader header frame already received
 *       In: psKind transfer kind expected in the header
 *       In: psDir folder where the file is stored
 *       Out: ppsPath path of the stored file (the caller frees it), or NULL
 * @Ret: TRANSFER_OK, TRANSFER_REJECTED or TRANSFER_LOST.
 *
 ***********************************************/
int receiveFileFrom(int nFd, const tFrame *pstHeader, const char *psKind, const char *psDir, char **ppsPath) {
    char sData[FRAME_DATA_SIZE + 1];
    char sMd5[MD5_LENGTH + 1];
    char sExpectedMd5[MD5_LENGTH + 1];
    char *psFileKind;
    char *psName;
    char *psSize;
    char *psMd5;
    char *psPath = NULL;
    int nSize;
    int nFrames;
    int nTotal = 0;
    int nFileFd;
    int nWriteOk = 1;
    tFrame stFrame;

    *ppsPath = NULL;

    // Header: "<KIND>&<FileName>&<FileSize>&<MD5SUM>"
    if (pstHeader->ucType != TYPE_FILE_HEADER) {
        return TRANSFER_LOST;
    }
    frameDataToString(pstHeader, sData, sizeof(sData));
    psFileKind = strtok(sData, "&");
    psName = strtok(NULL, "&");
    psSize = strtok(NULL, "&");
    psMd5 = strtok(NULL, "&");
    if (psFileKind == NULL || psName == NULL || psSize == NULL || psMd5 == NULL ||
        strcmp(psFileKind, psKind) != 0 || !isSafeName(psName) || !isMd5Text(psMd5) || atoi(psSize) <= 0) {
        // Nothing has been sent yet, so the connection stays in step
        sendTextFrame(nFd, TYPE_FILE_READY, FLAGS_ERR, "INVALID_HEADER");
        return TRANSFER_REJECTED;
    }
    nSize = atoi(psSize);
    strcpy(sExpectedMd5, psMd5);

    if (asprintf(&psPath, "%s/%s", psDir, psName) < 0) {
        sendTextFrame(nFd, TYPE_FILE_READY, FLAGS_ERR, "INTERNAL_ERROR");
        return TRANSFER_REJECTED;
    }
    nFileFd = open(psPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (nFileFd < 0) {
        free(psPath);
        sendTextFrame(nFd, TYPE_FILE_READY, FLAGS_ERR, "CANNOT_STORE_FILE");
        return TRANSFER_REJECTED;
    }
    if (sendTextFrame(nFd, TYPE_FILE_READY, FLAGS_OK, "READY") != 0) {
        close(nFileFd);
        unlink(psPath);
        free(psPath);
        return TRANSFER_LOST;
    }

    // N = FileSize / 253, rounded up
    nFrames = (nSize + FRAME_DATA_SIZE - 1) / FRAME_DATA_SIZE;
    for (int i = 0; i < nFrames; i++) {
        if (recvFrame(nFd, &stFrame) != 1 || stFrame.ucType != TYPE_FILE_DATA) {
            close(nFileFd);
            unlink(psPath);
            free(psPath);
            return TRANSFER_LOST;
        }
        if (write(nFileFd, stFrame.aucData, stFrame.ucDataLength) != stFrame.ucDataLength) {
            // Keep reading the frames so the connection stays in step
            nWriteOk = 0;
        }
        nTotal += stFrame.ucDataLength;
    }
    close(nFileFd);

    if (nTotal != nSize || !nWriteOk) {
        unlink(psPath);
        free(psPath);
        if (sendTextFrame(nFd, TYPE_FILE_ACK, FLAGS_ERR, "SIZE_ERROR") != 0) {
            return TRANSFER_LOST;
        }
        return TRANSFER_REJECTED;
    }
    if (computeMd5(psPath, sMd5) != 0 || strcasecmp(sMd5, sExpectedMd5) != 0) {
        unlink(psPath);
        free(psPath);
        if (sendTextFrame(nFd, TYPE_FILE_ACK, FLAGS_ERR, "MD5_ERROR") != 0) {
            return TRANSFER_LOST;
        }
        return TRANSFER_REJECTED;
    }

    if (sendTextFrame(nFd, TYPE_FILE_ACK, FLAGS_OK, "OK") != 0) {
        unlink(psPath);
        free(psPath);
        return TRANSFER_LOST;
    }
    *ppsPath = psPath;
    return TRANSFER_OK;
}