#include <stdio.h>
#include "tftp12FormatConvert.h"

#define FALSE 0
#define TRUE 1
#define ERROR -1
#define OK 1


/*
CONVERSION REGULAR:
lf(\n)->cr,lf(\r\n)
cr(\r)->cr,nul(\r\0)
*/
INT32 tftp12FileToAscii(INT32 *fd, UINT8 *pBuffer, INT32 bufferLen, UINT8 *charConv)
{
	UINT8 *ptr;		/*point to buffer*/
	UINT8 *currentChar; /*current char*/
	INT32 index;	/*count variable*/
	INT32 numBytes; /*numbytes read*/

	for (index = 0, ptr = pBuffer; index < bufferLen; ++index)
	{
		/*if previous char is \n or \r, convert it*/
		if ((*charConv == '\n') || (*charConv == '\r'))
		{
			currentChar = ((*charConv == '\n')? '\n':'\0');
			*charConv = '\0';
		}
		else
		{
			if ((numBytes = fread( &currentChar, sizeof(char), 1,fd)) == EOF)
			{
				break;
			}

			if (numBytes == 0)	/*end of file*/
			{
				break;
			}

			if ((currentChar == '\n') || (currentChar == '\r'))
			{
				*charConv = currentChar;
				currentChar = '\r';		/*put in \r first*/
			}
		}
		*ptr++ = currentChar;
	}

	return ((int)(ptr - pBuffer));
}

/*
CONVERT REGULAR:
cr,nul(\r\0)->cr(\r)
cr,lf(\r\n)->lf(\n)
*/
INT32 tftp12AsciiToFile(INT32 *fd, UINT8 *pBuffer, INT32 bufferLen, UINT8 *charConv, INT32 isLastBuff)
{
	INT32 count = bufferLen;	/*counter*/
	UINT8 *ptr = pBuffer;		/*point to buffer*/
	UINT8 currentChar;			/*current character*/
	INT32 skipWrite;			/*skip writing*/
	INT32 validBytes;           /*count number which was writen to file*/

	validBytes = 0;

	while (count--)
	{
		currentChar = *ptr++;
		skipWrite = FALSE;

		if (*charConv == '\r')
		{
			/*write out previous (\r) for anything but (\n).*/
			if (currentChar != '\n')
			{
				if (fwrite(&charConv, sizeof(char), 1, fd) <= 0)
				{
					return (ERROR);
				}
				validBytes++;
			}

			if (currentChar == '\0')	/*skip writing \0 for \r\0*/
			{
				skipWrite = TRUE;
			}
		}

		if ((!skipWrite) && (currentChar != '\r'))
		{
			if (fwrite(&currentChar, sizeof(char), 1, fd) <= 0)
			{
				return (ERROR);
			}
			validBytes++;
		}

		*charConv = currentChar;
	}/*end while*/

	/*
		since we wait to write the \r, if this is the last buffer and the last character
		is a \r, flush it now.
	*/
	if (isLastBuff && (*charConv == '\r'))
	{
		if (fwrite(charConv, sizeof(char), 1, fd) <= 0)
		{
			return (ERROR);
		}
		*charConv = '\0';
		validBytes++;
	}

	return validBytes;
}
