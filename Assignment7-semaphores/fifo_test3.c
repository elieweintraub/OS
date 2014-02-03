/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #7: fifo_test3.c                             */
/*****************************************************************************/

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "sem.h"
#include "fifo.h"

#define N_WORDS 50000  //number of words "produced" by each writer

void produce(struct fifo *fp,int n_words,int id);	
void consume(struct fifo *fp);

int my_procnum;

int main(int argc, char *argv[]){
	struct fifo f,*fp;
	int cpid1,cpid2,cpid3,i;
	
	//Establish shared memory region containing the shared fifo
	if((fp=mmap(NULL,sizeof f,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0))==MAP_FAILED){
		fprintf(stderr,"%s: Error using mmap(): %s\n",argv[0],strerror(errno));return -1;		
	}	
	memcpy(fp,&f,sizeof f);
	fifo_init(fp);

	//Create 4 virtual processors: a reader and 3 writers
	switch(cpid1=fork()){ //fork 1st writer process
		case -1:
			fprintf(stderr,"%s: Error using fork(): %s\n",argv[0],strerror(errno));return -1;
			break;
		case 0:  //CHILD PROCESS (writer1)
			my_procnum=1;
			produce(fp,N_WORDS,1);	
			return; 
			break;
		default: //PARENT PROCESS (reader)
			switch(cpid2=fork()){ //fork 2nd writer process
				case -1:
					fprintf(stderr,"%s: Error using fork(): %s\n",argv[0],strerror(errno));
					return -1;
					break;
				case 0:  //CHILD PROCESS (writer2)
					my_procnum=2;
					produce(fp,N_WORDS,2);	
					return; 
					break;
				default: //PARENT PROCESS (reader)
					switch(cpid3=fork()){ //fork 3rd writer process
						case -1:
							fprintf(stderr,"%s: Error using fork(): %s\n",argv[0],strerror(errno));
							return -1;
							break;
						case 0:  //CHILD PROCESS (writer3)
							my_procnum=3;
							produce(fp,N_WORDS,3);	
							return; 
							break;
						default: //PARENT PROCESS (reader)
							my_procnum=0;
							for(i=0;i<3*N_WORDS;i++){
								consume(fp);
							} 
							break;
					} //end 3rd writer process switch(fork())
					break;
			} //end 2nd writer process switch(fork())
			break;
	} //end 1st writer process switch(fork())	
	
	//Post program cleanup
	if(waitpid(cpid1,NULL,0)==-1 || waitpid(cpid2,NULL,0)==-1 || waitpid(cpid3,NULL,0)==-1){
		fprintf(stderr,"%s:Error using waitpid(): %s\n",argv[0],strerror(errno));return -1;
	}
	if(munmap(fp,sizeof f)<0){
		fprintf(stderr,"%s: Error using munmap(): %s\n",argv[0],strerror(errno));return -1;	
	}
	
	return 0;
}	

//produce 8 byte unsigned long the least significant 4 bytes of which correspond to the seq. num 
void produce(struct fifo *fp,int n_words,int id){
	int i;
	for(i=1;i<=n_words;i++) { fifo_wr(fp,((unsigned long)id<<(8*sizeof(int)))|i); }	
}

void consume(struct fifo *fp){
	unsigned long d=fifo_rd(fp);
	printf("Writer ID: %d, Sequence Number: %d\n",d>>(8*sizeof(int)),d&INT_MAX);
}	