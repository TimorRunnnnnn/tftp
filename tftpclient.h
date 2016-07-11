#ifndef _TFTPCLIENT_H_
#define _TFTPCLIENT_H_

#define EXIT() {\
				printf_s("\n程序发生致命错误,函数: %s",__FUNCTION__);\
				printf_s("\n在%s 的 %d 行",__FILE__,__LINE__);\
				printf_s("\n程序结束");\
				printf_s("\n");\
				system("Pause");\
				exit(42);\
					}






#endif