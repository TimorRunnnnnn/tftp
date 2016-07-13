#ifndef _TFTP12PACKET_H_
#define _TFTP12PACKET_H_

#include "windows.h"
#include "tftp.h"
/*计算缓冲区的大小，多一个字节是为了在recvfrom的时候，得知报文大小出错*/
#define TFTP12_BUFFER_SIZE(desc)		(((TFTP12Description_t *)desc)->option.blockSize+4+1)


INT32 tftp12CreateRequestPkt(TFTP12Description_t *desc);
INT32 tftp12ExtractOption(char *buffer,INT32 len, TFTP12Option *option);
#endif
