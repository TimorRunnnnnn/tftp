#include "tftp12log.h"



#define TFTP12_MAX_LOGMSG_SIZE			(512)
#define TFTP12_SHOW_LOG_BUFFER_SIZE		(2048)

#define TFTP12_LOG_SIZE_MAX				(10*1024)/*��־�ļ����1MB*/
#define TFTP12_LOG_SIZE_CHECK_COUNT		(500) /*ÿ500��дlog���һ���ļ���С*/

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
	//�ź���
}TFTP12LogNode;

struct
{
	INT32 initFlag;
	TFTP12LogNode client;
	TFTP12LogNode server;
	// 	INT32 tftp12ClinetLogEnable;
	// 	INT32 tftp12ServerLogEnable;
	// 	//�ͻ����ź�������
	// 	//�������ļ��ź�������
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


/*��Ҫ����ļ�����������*/
/*ÿ������һ����ļ�*/
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
		/*�����־�ļ���С����������ļ��������*/
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

			/*���м俪ʼ�����ҵ�����Ļ��з�������*/
			enterPos = realRead / 2;
			while ((logTem[enterPos] != '\n') && (enterPos < realRead))
			{
				enterPos++;
			}

			/*�ر��ļ���Ϊ����ļ���׼��*/
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

			/*�ر��ļ���������a+��ʽ��*/
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
 	tftp12ClientLogMsg("���Ĳ���\n");
 	tftp12ShowLogClient();
 	i = 100;
 	tftp12ClientLogMsg("testlog num:%d", i);
 	tftp12LogSizeCheck(&(TFTP12LogInfo.client));
 	tftp12ClientLogMsg("��׺��Ӳ���\n");
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

	/*���浱ǰ�ļ�ָ���λ��*/
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

	/*��ʼ����ʱ����һ����־�ļ���С*/
	tftp12LogSizeCheck(&(TFTP12LogInfo.client));
	tftp12LogSizeCheck(&(TFTP12LogInfo.server));
}


static void tftp12LogMsgMain(FILE *file, char *format, va_list ap)
{
	char msg[TFTP12_MAX_LOGMSG_SIZE];
	INT32 msgLen = 0;

	/*���һ����ֹ������ֹ����Խ�磬�����ڴ����㣬�˷�ʱ��*/
	msg[TFTP12_MAX_LOGMSG_SIZE - 1] = '\0';

	//��Ҫ���ʱ���

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
