#include "windows.h"
#include "tftp.h"
#include "stdarg.h"
#include "stdio.h"




char *tftp12BuildPacket(INT32 opcode, ...) 
{
	va_list ap;
	va_start(ap, opcode);
	if (opcode==TFTP12_READ_REQUEST)
	{
		char *filename = va_arg(ap, char*);
		printf("1:=%s", filename);
	}

	va_end(ap);
}