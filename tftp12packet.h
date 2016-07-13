#ifndef _TFTP12PACKET_H_
#define _TFTP12PACKET_H_

#include "windows.h"
#include "tftp.h"
/*���㻺�����Ĵ�С����һ���ֽ���Ϊ����recvfrom��ʱ�򣬵�֪���Ĵ�С����*/
#define TFTP12_BUFFER_SIZE(desc)		(((TFTP12Description_t *)desc)->option.blockSize+4+1)


INT32 tftp12CreateRequestPkt(TFTP12Description_t *desc);
INT32 tftp12ExtractOption(char *buffer,INT32 len, TFTP12Option *option);
#endif
