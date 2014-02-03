/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: sem.c                                    */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sem.h"

int tas(volatile char *lock);
void sigusr1_handler(int sn){/*N0_OP*/}

//Initialize the semaphore *s with the initial count.  
//Initialize any underlying data structures
void sem_init(struct sem *s, int count){
	int i;	
	s->count=count;                          //number of resources
	s->spinlock=0;                           //initially unlocked
	for(i=0;i<N_PROC;i++) s->waiting[i]=0;   //inialize waiting array 
	sigfillset(&s->mask);                    // mask for blocking all signals
	if(signal(SIGUSR1,sigusr1_handler)==SIG_ERR){
		fprintf(stderr,"Error setting SIGUSR1 handler\n");exit(-1);
	} 	
}

//Attempt to perform the "P" operation (atomically decrement the semaphore).
//If this operation would block, return 0, otherwise return 1.
int sem_try(struct sem *s){
	sigset_t old;
	sigprocmask(SIG_SETMASK,&s->mask,&old);  //block all signals
	while(tas(&s->spinlock)!=0){;}           //spin until the lock is acquired
	if(s->count==0){                         //sleep condition
		s->spinlock=0;                       //release the spinlock
		sigprocmask(SIG_SETMASK,&old,NULL);	 //restore original signal mask
		return 0;
	}
	s->count--;                  	         //decrement 
	s->spinlock=0;                  	     //release the spinlock 
	sigprocmask(SIG_SETMASK,&old,NULL);		 //restore original signal mask
	return 1;
}

//Perform the P operation, blocking until successful. 
void sem_wait(struct sem *s){
	sigset_t old,waitmask;
	sigfillset(&waitmask);
	sigdelset(&waitmask,SIGUSR1); 				 //waitmask masks all signals besides SIGUSR1
	sigprocmask(SIG_SETMASK,&s->mask,&old);  	 //block all signals
	while(1){
		while(tas(&s->spinlock)!=0){;}           //spin until the lock is acquired
		if(s->count==0){                         //sleep condition	
			s->waiting[my_procnum]=getpid();     //mark process for waiting
			s->spinlock=0;                       //release the spinlock                           
			sigsuspend(&waitmask);     		     //sleep
			continue;
		}
		s->count--;                             //decrement 
		s->spinlock=0;                          //release the spinlock
		break;
	}
	sigprocmask(SIG_SETMASK,&old,NULL);	    	//restore original signal mask
}

//Perform the V operation. If any processors were sleeping on this semaphore, wake them 
void sem_inc(struct sem *s){
	sigset_t old;
	int i;
	sigprocmask(SIG_SETMASK,&s->mask,&old);      //block all signals
	while(tas(&s->spinlock)!=0){;}               //spin until the lock is acquired	
	s->count++;                                  //increment 
	if(s->count==1){                             //wakeup condition
		for (i=0;i<N_PROC;i++){
			if(s->waiting[i]){                   //if process is waiting
				if(kill(s->waiting[i],SIGUSR1)<0){  //send wakeup signal
					perror("Error sending SIGUSR1 to wake process");exit(-1);
				}
				s->waiting[i]=0;                 //mark process as no longer waiting
			}
		}	
	}
	s->spinlock=0;                              //release the spinlock 	
	sigprocmask(SIG_SETMASK,&old,NULL);	        //restore original signal mask
}