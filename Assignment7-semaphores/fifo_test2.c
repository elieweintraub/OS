/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: fifo_test2.c  (acid test)                */
/*****************************************************************************/

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sem.h"
#include "fifo.h"

#define N_WRITERS  20    //number of writer processes
#define N_WORDS    5000  //number of words "produced" by each writer

int my_procnum;

int main(int argc, char *argv[]){
	struct fifo f,*fp;
	int i,n;
	unsigned long d;
	
	//Establish shared memory region containing the shared fifo
	if((fp=mmap(NULL,sizeof f,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0))==MAP_FAILED){
		fprintf(stderr,"%s: Error using mmap(): %s\n",argv[0],strerror(errno));return -1;		
	}	
	memcpy(fp,&f,sizeof f);
	fifo_init(fp);

	//Create virtual processors: a single reader and N_WRITERS writers
	for(n=1;n<=N_WRITERS;n++){
		switch(fork()){ //fork writer process
			case -1:
				fprintf(stderr,"%s: Error using fork(): %s\n",argv[0],strerror(errno));
				return -1;
				break; /*not reached*/
			case 0:  //CHILD PROCESS (writer n)
				my_procnum=n;
				for(i=1;i<=N_WORDS;i++) { fifo_wr(fp,((long)i<<P_MSK_SZ)|my_procnum); }		 
				return;
				break; /*not reached*/
			default: //PARENT PROCESS (reader)
				break;
		}
	}						
	my_procnum=0;
	for(i=0;i<N_WRITERS*N_WORDS;i++){
		d=fifo_rd(fp);printf("Writer ID: %d, Sequence Number: %d\n",d&(N_PROC-1),d>>P_MSK_SZ);
	} 
						
	//Post program cleanup
	for(n=1;n<=N_WRITERS;n++){
		if(wait(NULL)==-1){
			fprintf(stderr,"%s:Error using wait(): %s\n",argv[0],strerror(errno));return -1;
		}
	}	
	if(munmap(fp,sizeof f)<0){
		fprintf(stderr,"%s: Error using munmap(): %s\n",argv[0],strerror(errno));return -1;	
	}
	
	return 0;
}		