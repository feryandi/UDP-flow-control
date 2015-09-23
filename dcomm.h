/*
* File : dcomm.h
*/
#ifndef _DCOMM_H_

#define _DCOMM_H_

/* XON/XOFF protocol */
#define XON (0x11)
#define XOFF (0x13)
/* Const */
#define BYTESIZE 256 /* The maximum value of a byte */
#define MAXLEN 1024 /* Maximum messages length */

typedef unsigned char Byte;
typedef struct QTYPE
{
	unsigned int count;
	unsigned int front;
	unsigned int rear;
	unsigned int maxsize;
	Byte *data;
} QTYPE;

bool IsFull(QTYPE Q);
int EmptySpace(QTYPE Q);
void CreateQueue(QTYPE *Q, unsigned int size);
void Add(QTYPE *Q, Byte b);
Byte Del(QTYPE *Q);
void ViewContent(QTYPE *Q);

#endif









