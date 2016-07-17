#ifndef _TFTP12PACKET_H_
#define _TFTP12PACKET_H_

#include "windows.h"
#include "tftp12header.h"
/*计算缓冲区的大小，多一个字节是为了在recvfrom的时候，得知报文大小出错*/



INT32 tftp12CreateRequestPkt(TFTP12Description *desc);
INT32 tftp12ExtractOption(char *buffer,INT32 len, TFTP12Option *option);
#endif
