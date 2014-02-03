/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #8: sys_call_test.c                          */
/*                                                                           */
/*  sys_call_test - A simple test program that evaluates the relative  cost  */
/*  of empty loop iterations, user-mode function calls, and system calls     */
/*                                                                           */ 
/*****************************************************************************/

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

void empty_fnc(){/* NO_OP*/}

int main(int argc, char *argv[]){
	struct timespec start, end;
	double loop_cost,fnc_call_cost,sys_call_cost;	
	unsigned long int etime,i,n_itr=1e10;
	
	//Cost of Empty Loop Iteration
	if(clock_gettime(CLOCK_REALTIME,&start)==-1){perror("Error using clock_gettime()");return -1;}
	for(i=0;i<n_itr;i++){;}
	if(clock_gettime(CLOCK_REALTIME,&end)==-1){perror("Error using clock_gettime()");return -1;}	
	etime=(end.tv_sec-start.tv_sec)*1e9 +(end.tv_nsec-start.tv_nsec);
	loop_cost=etime/(double)n_itr;
	printf("Cost of empty loop iteration: %f ns\n",loop_cost);
	
	//Cost of Empty Function Call
	if(clock_gettime(CLOCK_REALTIME,&start)==-1){perror("Error using clock_gettime()");return -1;}
	for(i=0;i<n_itr;i++) {empty_fnc();}
	if(clock_gettime(CLOCK_REALTIME,&end)==-1){perror("Error using clock_gettime()");return -1;}	
	etime=(end.tv_sec-start.tv_sec)*1e9 +(end.tv_nsec-start.tv_nsec);
	fnc_call_cost=etime/(double)n_itr - loop_cost;
	printf("Cost of empty function call: %f ns\n",fnc_call_cost);
	
	//Cost of Simple System Call (getuid)
	if(clock_gettime(CLOCK_REALTIME,&start)==-1){perror("Error using clock_gettime()");return -1;}
	for(i=0;i<n_itr;i++) {getuid();}
	if(clock_gettime(CLOCK_REALTIME,&end)==-1){perror("Error using clock_gettime()");return -1;}	
	etime=(end.tv_sec-start.tv_sec)*1e9 +(end.tv_nsec-start.tv_nsec);
	sys_call_cost=etime/(double)n_itr - loop_cost - fnc_call_cost;
	printf("Cost of simple system call: %f ns\n",sys_call_cost);
	printf("A simple system call is %f times more expensive than an empty function call\n",sys_call_cost/fnc_call_cost);
		
	return 0;
}