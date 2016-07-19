#pragma once
#include "tftp12header.h"

INT32 tftp12ServerEnable(void);
void tftp12ServerInit(void);
void tftp12SeverShellCallback(INT32 argc, INT8 *argv[]);
void tftp12ServerShowStatus(void);