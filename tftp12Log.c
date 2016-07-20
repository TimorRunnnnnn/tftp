#include "tftp12log.h"



#define TFTP12_MAX_LOGMSG_SIZE			(512)
#define TFTP12_SHOW_LOG_BUFFER_SIZE		(2048)

#define TFTP12_LOG_SIZE_MAX				(10*1024)/*日志文件最大1MB*/
#define TFTP12_LOG_SIZE_CHECK_COUNT		(500) /*每500次写log检查一次文件大小*/

#define TFTP12_SERVER_LOG_FILE			(1)
#define TFTP12_CLIENT_LOG_FILE			(0)

static char* const fileName[] = {
	"tftp12ClinetLog.txt",
	"tftp12ServerLog.txt",
};

typedef struct _tftp12lognode
{
	INT32 enable;
	INT32 fileNameID;
	FILE *file;
	INT32 sizeCheckCount;
	//信号量
}TFTP12LogNode;

struct
{
	INT32 initFlag;
	TFTP12LogNode client;
	TFTP12LogNode server;
	// 	INT32 tftp12ClinetLogEnable;
	// 	INT32 tftp12ServerLogEnable;
	// 	//客户端信号量保护
	// 	//服务器文件信号量保护
	// 
	// 	INT32 sizeCheckCount;
	// 	FILE *clientLogFile;
	// 	FILE *serverLogFile;
}TFTP12LogInfo;

static void tftp12ShowLog(FILE *logfile);


// static void tftp12LogClear(FILE node->file)
// {
// 
// }


/*需要添加文件描述符的锁*/
/*每次清理一半的文件*/
static void tftp12LogSizeCheck(TFTP12LogNode *node)
{
	INT32 fileSize = 0;
	INT32 realRead = 0;
	INT32 realWrite = 0;
	INT32 i = 0;
	INT32 enterPos = 0;
	INT8 *logTem = NULL;

	//semTake(server)
	if (node->file != NULL)
	{
		fseek(node->file, 0, SEEK_END);
		fileSize = ftell(node->file);
		fseek(node->file, 0, SEEK_SET);
		/*如果日志文件大小大于了最大文件体积限制*/
		if (fileSize > TFTP12_LOG_SIZE_MAX)
		{
			logTem = (INT8 *)malloc(fileSize + 1);
			if (logTem == NULL)
			{
				printf("\ntftp12LogSizeCheck malloc failed");
				return;
			}
			realRead = fread(logTem, 1, fileSize, node->file);
			if (realRead <= 0)
			{
				return;
			}

			/*从中间开始往后找到最近的换行符，剪掉*/
			enterPos = realRead / 2;
			while ((logTem[enterPos] != '\n') && (enterPos < realRead))
			{
				enterPos++;
			}

			/*关闭文件，为清空文件做准备*/
			FCLOSE_Z(node->file);
			node->file = fopen(fileName[node->fileNameID], "w+");
			if (node->file != NULL)
			{
				realWrite = fwrite(&logTem[enterPos + 1], 1, ((realRead - (enterPos + 1))), node->file);
				if (realWrite != ((realRead - (enterPos + 1))))
				{
					printf("\nlog size check write bytes error");
				}
			}

			/*关闭文件，重新以a+方式打开*/
			FCLOSE_Z(node->file);
			node->file = fopen(fileName[node->fileNameID], "a+");
			if (node->file == NULL)
			{
				printf("\nlog file check reopen failed");
				node->enable = FALSE;
			}
			free(logTem);
		}
	}
	//semGiVe(server);
}

 void logtest()
 {
 	INT32 i = 0;
 	tftp12LogInit();
 	tftp12ClientLogMsg("testlog num:%d", i);
 	i++;
 	tftp12ClientLogMsg("testlog num:%d", i);
 	i++;
 	tftp12ClientLogMsg("testlog num:%d", i);
 	i++;
 	tftp12ClientLogMsg("testlog num:%d", i);
 	tftp12ClientLogMsg("中文测试\n");
 	tftp12ShowLogClient();
 	i = 100;
 	tftp12ClientLogMsg("testlog num:%d", i);
 	tftp12LogSizeCheck(&(TFTP12LogInfo.client));
 	tftp12ClientLogMsg("后缀添加测试\n");
 }

static void tftp12ShowLog(FILE *logfile)
{
	INT32 i = 0;
	INT32 readBytes = 0;
	INT32 logFileCurrentPos = 0;
	INT8 *readBuf = (INT8 *)malloc(TFTP12_SHOW_LOG_BUFFER_SIZE);
	if (readBuf == NULL)
	{
		printf("\nMemory alloc failed!");
		return;
	}
	if (logfile == NULL)
	{
		printf("\nLog file is NULL");
		return;
	}
	//semTake()

	/*保存当前文件指针的位置*/
	logFileCurrentPos = ftell(logfile);
	fseek(logfile, 0, SEEK_SET);
	readBytes = TFTP12_SHOW_LOG_BUFFER_SIZE;
	while (readBytes == TFTP12_SHOW_LOG_BUFFER_SIZE)
	{
		readBytes = fread(readBuf, 1, TFTP12_SHOW_LOG_BUFFER_SIZE, logfile);
		if (readBytes > 0)
		{
			for (i = 0; i < readBytes; i++)
			{
				printf("%c", readBuf[i]);
			}
		}
	}
	fseek(logfile, logFileCurrentPos, SEEK_SET);
	free(readBuf);
}

void tftp12ShowLogClient(void)
{
	tftp12ShowLog(TFTP12LogInfo.client.file);
}

void tftp12ShowLogServer(void)
{
	tftp12ShowLog(TFTP12LogInfo.server.file);
}

void tftp12LogInit(void)
{
	if (TFTP12LogInfo.initFlag == TRUE)
	{
		return;
	}
	memset(&TFTP12LogInfo, 0, sizeof(TFTP12LogInfo));

	TFTP12LogInfo.client.fileNameID = TFTP12_CLIENT_LOG_FILE;
	TFTP12LogInfo.server.fileNameID = TFTP12_SERVER_LOG_FILE;
	TFTP12LogInfo.client.file = fopen(fileName[TFTP12LogInfo.client.fileNameID], "a+");
	TFTP12LogInfo.server.file = fopen(fileName[TFTP12LogInfo.server.fileNameID], "a+");

	if (TFTP12LogInfo.client.file == NULL)
	{
		printf("\nopen client logfile failed");
	}
	else
	{
		TFTP12LogInfo.client.enable = TRUE;
	}

	if (TFTP12LogInfo.server.file == NULL)
	{
		printf("\nopen server logfile failed");
	}
	else
	{
		TFTP12LogInfo.server.enable = TRUE;
	}
	TFTP12LogInfo.initFlag = TRUE;

	/*初始化的时候检查一次日志文件大小*/
	tftp12LogSizeCheck(&(TFTP12LogInfo.client));
	tftp12LogSizeCheck(&(TFTP12LogInfo.server));
}


static void tftp12LogMsgMain(FILE *file, char *format, va_list ap)
{
	char msg[TFTP12_MAX_LOGMSG_SIZE];
	INT32 msgLen = 0;

	/*添加一个终止符，防止访问越界，不用内存清零，浪费时间*/
	msg[TFTP12_MAX_LOGMSG_SIZE - 1] = '\0';

	//需要添加时间戳

	_vsnprintf(msg, TFTP12_MAX_LOGMSG_SIZE, format, ap);
	msgLen = strlen(msg);
	msg[msgLen] = '\n';
	msg[msgLen + 1] = '\0';

	fputs(msg, file);
	fflush(file);

	return;
}

void tftp12ClientLogMsg(char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	if (TFTP12LogInfo.client.enable == TRUE)
	{
		//semTake()
		tftp12LogMsgMain(TFTP12LogInfo.client.file, format, ap);
		//semGive();	
	}
	va_end(ap);
}



void tftp12ServerLogMsg(char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	if (TFTP12LogInfo.server.enable == TRUE)
	{
		//semTake()
		tftp12LogMsgMain(TFTP12LogInfo.server.file, format, ap);
		//semGive();
	}
	va_end(ap);
}

void tftp12ServerLogTask(void *arg)
{

}
