#ifndef _TFTPCLIENT_H_
#define _TFTPCLIENT_H_

#define EXIT() {\
				printf_s("\n��������������,����: %s",__FUNCTION__);\
				printf_s("\n��%s �� %d ��",__FILE__,__LINE__);\
				printf_s("\n�������");\
				printf_s("\n");\
				system("Pause");\
				exit(42);\
					}






#endif