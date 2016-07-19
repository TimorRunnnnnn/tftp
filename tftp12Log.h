#pragma once
#include "tftp12header.h"



void logtest();

void tftp12LogInit(void);
void tftp12ClientLogMsg(char * format, ...);
void tftp12ServerLogMsg(char * format, ...);

void tftp12ShowLogClient(void);
void tftp12ShowLogServer(void);