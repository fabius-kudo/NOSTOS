#ifndef CODE_PROTOCOL_H
#define CODE_PROTOCOL_H

#include <unistd.h>
#include <string.h>
#include <errno.h>

#define FRAME_SIZE      256
#define FRAME_DATA_SIZE 253

// Frame types
#define TYPE_CONNECT_ITHACA 0x01
#define TYPE_LIST_VOYAGES   0x02
#define TYPE_ACCEPT         0x03
#define TYPE_CLAIM          0x04
#define TYPE_DISCONNECT     0x05
#define TYPE_ISLAND_ARRIVE  0x10
#define TYPE_LIST_MARKET    0x11
#define TYPE_BUY            0x12
#define TYPE_SELL           0x13
#define TYPE_BUY_MAP        0x14
#define TYPE_DELIVER        0x15
#define TYPE_ISLAND_LEAVE   0x16
#define TYPE_FILE_HEADER    0x20
#define TYPE_FILE_DATA      0x21
#define TYPE_FILE_READY     0x22
#define TYPE_FILE_ACK       0x23
#define TYPE_NACK           0x7F

// Flags
#define FLAG_REQUEST  0x00
#define FLAG_RESPONSE 0x80
#define FLAG_ERROR    0x40
#define FLAG_BINARY   0x20
#define FLAGS_OK      (FLAG_RESPONSE)
#define FLAGS_ERR     (FLAG_RESPONSE | FLAG_ERROR) //TODO: what dis mean

typedef struct {
    unsigned char ucType;
    unsigned char ucFlags;
    unsigned char ucDataLength;
    unsigned char aucData[FRAME_DATA_SIZE];
} tFrame;

int buildFrame(unsigned char *pucBuf, unsigned char ucType, unsigned char ucFlags, const void *pData, int nLen);
void parseFrame(const unsigned char *pucBuf, tFrame *pstFrame);
int sendFrame(int nFd, unsigned char ucType, unsigned char ucFlags, const void *pData, int nLen);
int sendTextFrame(int nFd, unsigned char ucType, unsigned char ucFlags, const char *psText);
int recvFrame(int nFd, tFrame *pstFrame);
void frameDataToString(const tFrame *pstFrame, char *psDest, int nDestSize);

#endif