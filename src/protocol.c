#include "protocol.h"

/***********************************************
 * @Name: buildFrame
 * @Def: Serializes a frame into a 256-byte buffer, zero-padded.
 * @Ret: 0 on success, -1 if the data does not fit.
 ***********************************************/

//TODO: check how frames are built in given examples
int buildFrame(unsigned char *pucBuf, unsigned char ucType, unsigned char ucFlags, const void *pData, int nLen) {
    if (nLen < 0 || nLen > FRAME_DATA_SIZE) {
        return -1;
    }
    memset(pucBuf, 0, FRAME_SIZE);          // padding is always 0x00
    pucBuf[0] = ucType;
    pucBuf[1] = ucFlags;
    pucBuf[2] = (unsigned char)nLen;
    if (nLen > 0 && pData != NULL) {
        memcpy(&pucBuf[3], pData, nLen);
    }
    return 0;
}

/***********************************************
 * @Name: parseFrame
 * @Def: Copies a received 256-byte buffer into a tFrame.
 ***********************************************/
void parseFrame(const unsigned char *pucBuf, tFrame *pstFrame) {
    pstFrame->ucType = pucBuf[0];
    pstFrame->ucFlags = pucBuf[1];
    pstFrame->ucDataLength = pucBuf[2];
    if (pstFrame->ucDataLength > FRAME_DATA_SIZE) {
        pstFrame->ucDataLength = FRAME_DATA_SIZE;   // never trust the peer
    }
    memcpy(pstFrame->aucData, &pucBuf[3], FRAME_DATA_SIZE);
}

/***********************************************
 * @Name: sendFrame
 * @Def: Builds a frame and sends it with a single write of 256 bytes.
 * @Ret: 0 on success, -1 on error.
 ***********************************************/
int sendFrame(int nFd, unsigned char ucType, unsigned char ucFlags, const void *pData, int nLen) {
    unsigned char aucBuf[FRAME_SIZE];

    if (buildFrame(aucBuf, ucType, ucFlags, pData, nLen) < 0) {
        return -1;
    }
    if (write(nFd, aucBuf, FRAME_SIZE) != FRAME_SIZE) {
        return -1;
    }
    return 0;
}

/***********************************************
 * @Name: sendTextFrame
 * @Def: Convenience wrapper for text data (e.g. "OK", "name&ip&port").
 ***********************************************/
int sendTextFrame(int nFd, unsigned char ucType, unsigned char ucFlags, const char *psText) {
    return sendFrame(nFd, ucType, ucFlags, psText, (int)strlen(psText));
}    

/***********************************************
 * @Name: recvFrame
 * @Def: Reads exactly one 256-byte frame.
 * @Ret: 1 on success, 0 if the peer closed the connection, -1 on error
 *       (errno == EINTR if interrupted by a signal such as CTRL+C).
 ***********************************************/
int recvFrame(int nFd, tFrame *pstFrame) {
    //TODO: idk why we need the buffer
    unsigned char aucBuf[FRAME_SIZE];
    int nTotal = 0;
    ssize_t nRead;

    while (nTotal < FRAME_SIZE) {
        nRead = read(nFd, aucBuf + nTotal, FRAME_SIZE - nTotal);
        if (nRead == 0) {
            return 0;                       // disconnected
        }
        if (nRead < 0) {
            return -1;                      // caller checks errno/EINTR
        }
        nTotal += (int)nRead;
    }
    parseFrame(aucBuf, pstFrame);
    return 1;
}

/***********************************************
 * @Name: frameDataToString
 * @Def: Copies the valid data bytes into a NUL-terminated string so the
 *       fields can be split on '&' with strtok_r.
 ***********************************************/
void frameDataToString(const tFrame *pstFrame, char *psDest, int nDestSize) {
    int nLen = pstFrame->ucDataLength;

    if (nLen > nDestSize - 1) {
        nLen = nDestSize - 1;
    }
    memcpy(psDest, pstFrame->aucData, nLen);
    psDest[nLen] = '\0';
}