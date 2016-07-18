#pragma comment(lib, "ws2_32.lib")
#include "winsock2.h"
#include "ws2tcpip.h"
#include "windows.h"
#include "stdio.h"
#include "stdint.h"
#include "conio.h"
#include "time.h"

#include "tftpclient.h"
#include "tftp12packet.h"
#include "tftp12header.h"
#include "tftp12Server.h"
static inline tftp12() {

}


int main()
{
	INT16 wVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	INT32 err = WSAStartup(wVersion, &wsaData);

	if (err != 0)
	{
		printf("\nWSAStartup failed");
		EXIT();
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("\ncould not find a usable version of Winsock.dll");
		WSACleanup();
		EXIT();
	}

	tftp12LogInit();
	tftp12ClientLogMsg("testlog:%d...%d.%s;", 123,123222, "testStr");
	extern INT32 tftp12ServerEnable();
	extern void tftp12ServerShowStatus(void);
	tftp12ServerInit();
	tftp12ServerEnable();
	//extern void testtrans();
 	//testtrans();
 	//test();

	char input[512];
	INT32 pos = 0;
	while (1)
	{
		input[pos] = _getch();
		tftp12ServerShowStatus();
		printf("%c",input[pos]);
		if (input[pos]=='\r')
		{
			pos = 0;
			
			//tftp12ParseCommand(input);
			printf("\n");
		}
		pos++;
	}

    return 0;
}

