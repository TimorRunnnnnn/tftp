#pragma once
#include "tftp12header.h"


#define FREE_Z(m)	{\
						if(m!=NULL)		\
						{				\
							free(m);	\
							m=NULL;		\
						}				\
					}				

#define FCLOSE_Z(f)	{\
						if(f!=NULL)		\
						{				\
							fclose(f);	\
							f=NULL;		\
						}				\
					}		

#define SCLOSE_Z(s)	{\
						if(s!=0)		\
						{				\
							closesocket(s);	\
							s=0;		\
						}				\
					}				


INT32 tftp12ServerEnable(void);
void tftp12ServerInit(void);
void tftp12SeverShellCallback(INT32 argc, INT8 *argv[]);
void tftp12ServerShowStatus(void);