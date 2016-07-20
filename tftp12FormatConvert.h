#ifndef _TFTP12_FORMATCONVERT_H_
#define _TFTP12_FORMATCONVERT_H_
typedef int INT32;
typedef unsigned char UINT8;
/*
CONVERSION REGULAR:
lf(\n)->cr,lf(\r\n)

cr(\r)->cr,nul(\r\0)
*/
INT32 tftp12FileToAscii(INT32 *fd, UINT8 *pBuffer, INT32 bufferLen, UINT8 *charConv);
INT32 tftp12AsciiToFile(INT32 *fd, UINT8 *pBuffer, INT32 bufferLen, UINT8 *charConv, INT32 isLastBuff);





#endif
