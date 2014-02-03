/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: sem.h                                    */
/*****************************************************************************/

#ifndef _SEM_H
#define _SEM_H

#include <signal.h>

#define N_PROC 64          //should be a power of 2
#define P_MSK_SZ 6         //log2(N_PROC)

//virtual processor number
extern int my_procnum;

//struct sem definition
struct sem {
	int count;            //number of "resources" available
	char spinlock;         
	int waiting[N_PROC];  //contains pid if waitng, 0 if not waiting
	sigset_t mask;
};

//Initialize the semaphore *s with the initial count. 
//sem_init should only be called once in the program. 
void sem_init(struct sem *s, int count);

//Attempt to perform the "P" operation (atomically decrement the semaphore).
//If this operation would block, return 0, otherwise return 1.
int sem_try(struct sem *s);

//Perform the P operation, blocking until successful. 
void sem_wait(struct sem *s);

//Perform the V operation.
//If any other processors were sleeping on this semaphore, wake them 
void sem_inc(struct sem *s);

#endif //_SEM_H