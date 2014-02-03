/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: fifo.h                                   */
/*****************************************************************************/

#ifndef _FIFO_H
#define _FIFO_H

#include "sem.h"

#define MYFIFO_BUFSIZ  4096

//struct fifo definition
struct fifo {
	unsigned long buf[MYFIFO_BUFSIZ];
	int next_write,next_read;
	struct sem rd_sem,wr_sem,mutex; //mutex proects buf, next_write, and next_read
};

//Initialize the shared memory FIFO *f including any
//required underlying initializations.
void fifo_init(struct fifo *f);

//Enqueue the data word d into the FIFO, blocking
//unless and until the FIFO has room to accept it. 
void fifo_wr(struct fifo *f,unsigned long d);

//Dequeue the next data word from the FIFO and return it. Block
//unless and until there are available words queued in the FIFO.
unsigned long fifo_rd(struct fifo *f);

#endif //_FIFO_H