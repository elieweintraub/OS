/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: fifo_test1.c    (basic test)             */
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

int my_procnum;

int main(int argc, char *argv[]){
	struct fifo f,*fp;
	int cpid,status,i;	
	//Establish shared memory region containing the shared fifo
	if((fp=mmap(NULL,sizeof f,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0))==MAP_FAILED){
		fprintf(stderr,"%s: Error using mmap(): %s\n",argv[0],strerror(errno));return -1;		
	}	
	memcpy(fp,&f,sizeof f);	
	fifo_init(fp);
	//Create 2 virtual processes: a reader and a writer
	switch(cpid=fork()){
		case -1:
			fprintf(stderr,"%s: Error using fork(): %s\n",argv[0],strerror(errno));return -1;
			break; /*not reached*/
		case 0:  //CHILD PROCESS (writer)
			my_procnum=1;
			for(i=1;i<=10000;i++){fifo_wr(fp,i);}	
			return; 
			break; /*not reached*/
		default: //PARENT PROCESS (reader)
			my_procnum=0;
			for(i=1;i<=10000;i++){printf("%d: %ld\n",i,fifo_rd(fp));}	
			break;
	}	
	//Post program cleanup
	if(waitpid(cpid,&status,0)==-1){
		fprintf(stderr,"%s:Error using waitpid(): %s\n",argv[0],strerror(errno));return -1;
	}
	if(munmap(fp,sizeof f)<0){
		fprintf(stderr,"%s: Error using munmap(): %s\n",argv[0],strerror(errno));return -1;	
	}	
	return 0;
}	