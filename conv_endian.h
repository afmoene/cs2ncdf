/*
 * ** This program works ONLY for
 * ** Little-Endian and Big-Endian files/processors
 * ** NOT for Middle-Endian
 * */
/* This code originates from:
 * http://www.thescripts.com/forum/thread201006.html, posted by Marco de Boer at Nov. 13, 2005 */

#include <stdio.h>

typedef enum {
	ENDIAN_NO_INFORMATION,
	ENDIAN_LITTLE,
	ENDIAN_MIDDLE,
	ENDIAN_BIG
} ENDIAN_TYPE;


static ENDIAN_TYPE UtilEndianType()
{
	ENDIAN_TYPE EndianType=ENDIAN_NO_INFORMATION;
	unsigned long Value=0x12345678;
	unsigned char *cPtr=(unsigned char*)&Value;

	if ( *cPtr==0x12 && *(cPtr+1)==0x34 && *(cPtr+2)==0x56 && *(cPtr+3)==0x78 )
		EndianType=ENDIAN_BIG;
	else if ( *cPtr==0x78 && *(cPtr+1)==0x56 && *(cPtr+2)==0x34 &&
			*(cPtr+3)==0x12 )
		EndianType=ENDIAN_LITTLE;
	else if ( *cPtr==0x34 && *(cPtr+1)==0x12 && *(cPtr+2)==0x78 &&
			*(cPtr+3)==0x56 )
		EndianType=ENDIAN_MIDDLE;
	return EndianType;
}

static void *ReverseBytesInArray(void *Buffer,size_t Size)
{
	unsigned char *cPtr0,*cPtr1,cTmp;

	cPtr0=Buffer;
	cPtr1=cPtr0+Size;
	while ( cPtr0<--cPtr1 ) {
		cTmp=*cPtr0;
		*cPtr0++=*cPtr1;
		*cPtr1=cTmp;
	}
	return Buffer;
}
