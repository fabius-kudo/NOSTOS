/***********************************************
 *
 * @File : protocol.c
 * @Purpose : Frame handling for the Nostos protocol. Every message is a fixed 256-byte frame
 * @Author : Elvar Nói Leistner, Daiki Fabius Kudo
 * @Date : 3/10/26
 *
 ***********************************************/

#include "protocol.h"

/***********************************************
 *
 * @Name: buildFrame
 * @Def: Serializes a frame into a 256-byte buffer.
 * @Arg: Out: pucBuf buffer of at least FRAME_SIZE bytes
 *       In: ucType frame type (TYPE_*)
 *       In: ucFlags frame flags (FLAG_* / FLAGS_*)
 *       In: pData data to copy, may be NULL when nLen is 0
 *       In: nLen number of valid data bytes (0 to 253)
 * @Ret: 0 on success, -1 if the data does not fit in a frame.
 *
 ***********************************************/
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
 *
 * @Name: parseFrame
 * @Def: Deserializes a received 256-byte buffer into a tFrame. The data
 *       length is clamped to 253, so a malformed frame from the peer can
 *       never make later code read past the data field.
 * @Arg: In: pucBuf buffer of FRAME_SIZE bytes as received from the socket
 *       Out: pstFrame frame to fill
 * @Ret: None.
 *
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
 *
 * @Name: sendFrame
 * @Def: Builds a frame and sends it with a single write of 256 bytes, as
 *       the protocol requires.
 * @Arg: In: nFd connected socket
 *       In: ucType frame type
 *       In: ucFlags frame flags
 *       In: pData data to send, may be NULL when nLen is 0
 *       In: nLen number of valid data bytes (0 to 253)
 * @Ret: 0 on success, -1 if the data does not fit or the write failed
 *       (for example, the peer closed the connection).
 *
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
 *
 * @Name: sendTextFrame
 * @Def: Sends a frame whose data is a text string, such as "OK", a reason,
 *       or '&'-separated fields. The terminating '\0' is not sent;
 *       DATA_LENGTH tells the receiver where the text ends.
 * @Arg: In: nFd connected socket
 *       In: ucType frame type
 *       In: ucFlags frame flags
 *       In: psText NUL-terminated text, at most 253 characters
 * @Ret: 0 on success, -1 on error.
 *
 ***********************************************/
int sendTextFrame(int nFd, unsigned char ucType, unsigned char ucFlags, const char *psText) {
    return sendFrame(nFd, ucType, ucFlags, psText, (int)strlen(psText));
}    

/***********************************************
 *
 * @Name: recvFrame
 * @Def: Reads exactly one 256-byte frame. The bytes are first collected in
 *       a raw buffer and then deserialized with parseFrame.
 * @Arg: In: nFd connected socket
 *       Out: pstFrame received frame
 * @Ret: 1 on success, 0 if the peer closed the connection, -1 on error.
 *
 ***********************************************/
int recvFrame(int nFd, tFrame *pstFrame) {
    unsigned char aucBuf[FRAME_SIZE];
    int nTotal = 0;
    ssize_t nRead;

    while (nTotal < FRAME_SIZE) {
        nRead = read(nFd, aucBuf + nTotal, FRAME_SIZE - nTotal);
        if (nRead == 0) {
            return 0;                       // disconnected
        }
        if (nRead < 0) {
            return -1;                      
        }
        nTotal += (int)nRead;
    }
    parseFrame(aucBuf, pstFrame);
    return 1;
}

/***********************************************
 *
 * @Name: frameDataToString
 * @Def: Copies the valid data bytes of a text frame into a NUL-terminated
 *       string, so its fields can be split on '&'. Text longer than the
 *       destination is truncated.
 * @Arg: In: pstFrame received frame
 *       Out: psDest destination string
 *       In: nDestSize size of psDest in bytes (FRAME_DATA_SIZE + 1 is enough)
 * @Ret: None.
 *
 ***********************************************/
void frameDataToString(const tFrame *pstFrame, char *psDest, int nDestSize) {
    int nLen = pstFrame->ucDataLength;

    if (nLen > nDestSize - 1) {
        nLen = nDestSize - 1;
    }
    memcpy(psDest, pstFrame->aucData, nLen);
    psDest[nLen] = '\0';
}