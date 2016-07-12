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
#include "tftp.h"

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


	test();

	char input[10];
	while (1)
	{
		scanf_s("%s", input, 10);
		if (strcmp(input, "send") == 0)
		{
			//send
		}
		else if (strcmp(input, "put") == 0)
		{
			//put
		}
	}

    return 0;
}

