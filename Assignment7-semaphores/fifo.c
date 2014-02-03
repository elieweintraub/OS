/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: fifo.c                                   */
/*****************************************************************************/

#include "fifo.h"

//Initialize the shared memory FIFO 
void fifo_init(struct fifo *f){
	f->next_write=0;                       //index of next write            
	f->next_read=0;                        //index of next read  
	sem_init(&f->wr_sem,MYFIFO_BUFSIZ);    //write semaphore contains # of free write slots
	sem_init(&f->rd_sem,0);                //read semaphore contains # of free read slots
	sem_init(&f->mutex,1);                 //mutex initially unlocked 
}

//Enqueue d into the FIFO, blocking until the FIFO has room to accept it. 
void fifo_wr(struct fifo *f,unsigned long d){
	sem_wait(&f->wr_sem);                  //wait for an available slot to write
	sem_wait(&f->mutex);             	   //obtain the mutex
	f->buf[f->next_write++]=d;             //perform the write
	f->next_write%=MYFIFO_BUFSIZ;
	sem_inc(&f->mutex);                    //release the mutex
	sem_inc(&f->rd_sem);                   //increment the number of available reading slots 
}

//Dequeue and return the next data word, blocking until there are available words.
unsigned long fifo_rd(struct fifo *f){
	unsigned long d;
	sem_wait(&f->rd_sem);          		   //wait for an available slot to read
	sem_wait(&f->mutex);            	   //obtain the mutex
	d=f->buf[f->next_read++];              //perform the read
	f->next_read%=MYFIFO_BUFSIZ;
	sem_inc(&f->mutex);                    //release the mutex
	sem_inc(&f->wr_sem);                   //increment the number of available writing slots
	return d;
}