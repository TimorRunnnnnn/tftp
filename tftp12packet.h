#ifndef _TFTP12PACKET_H_
#define _TFTP12PACKET_H_

#include "windows.h"
#include "tftp12header.h"
/*���㻺�����Ĵ�С����һ���ֽ���Ϊ����recvfrom��ʱ�򣬵�֪���Ĵ�С����*/



INT32 tftp12CreateRequestPkt(TFTP12Description *desc);
INT32 tftp12ExtractOption(char *buffer,INT32 len, TFTP12Option *option);
#endif
