#pragma once

#include "windows.h"
#include "tftp12header.h"

INT32 tftp12SendAndRecv(TFTP12Description *desc, INT32 SendPktSize);
