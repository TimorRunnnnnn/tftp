#pragma once

#include "windows.h"
#include "tftp12header.h"

INT32 tftp12SendAndRecv(TFTP12Description *desc, char *sendBuf, INT32 SendPktSize, INT32 *recvBytes, INT32 lastPkt);