/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #4: catgrepmore.c                            */
/*                                                                           */
/*  catgrepmore- emulates cat|grep|more pipeline                             */
/*                                                                           */
/*  USAGE:                                                                   */
/*		catgrepmore  pattern infile1 [...infile2...]                         */ 
/*                                                                           */ 
/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096

int n_files,n_bytes=0;

void int_handler(int sn);    	 			
int my_read(int fd, void *buf, size_t count, char *in, char **argv);
int my_write(int fd, const void *buf, size_t count, char **argv);

int main(int argc, char *argv[]){

	int p1[2], p2[2], cpid1, cpid2, fd, status, n, r;	
	char buf[BUF_SIZE], *fn, *pattern, **infiles;
	char *usage="usage: catgrepmore  pattern infile1 [...infile2...]\n";
		
	//Parse command line arguments 
	if (argc<3){
		fprintf(stderr,"%s: Not enough command line arguments!\n%s",argv[0],usage);
		exit(-1);
	}
	pattern=argv[1];  infiles=argv+2; 	
	//Set up signal handling
	if(signal(SIGINT,int_handler)==SIG_ERR){
		fprintf(stderr,"%s: Error setting SIGINT handler\n",argv[0]);
		exit(-1);
	}
	if(signal(SIGPIPE,SIG_IGN)==SIG_ERR){
		fprintf(stderr,"%s: Error setting disposition of SIGPIPE to SIG_IGN\n",argv[0]);
		exit(-1);
	}
	//For each infile...
	for(n_files=0;fn=infiles[n_files];n_files++){	
		//Create pipeline pipes (p1:infile_content|grep  p2:grep_output|more)
		if(pipe(p1)<0 || pipe(p2)<0){
			fprintf(stderr,"%s: Error creating pipeline: %s\n",argv[0],strerror(errno)); exit(-1);
		}
		//Open infile
		if((fd=open(fn,O_RDONLY))<0){ 
			fprintf(stderr,"%s: Can't open file %s for reading: %s\n",argv[0],fn,strerror(errno));
			continue;		
		}
		switch(cpid1=fork()){
			case -1:
				fprintf(stderr,"%s: Grep fork failed: %s\n",argv[0],strerror(errno));
				exit(-1);
				break;
			case 0:  //CHILD PROCESS (grep)
    			//Perform  I/O redirection
				if(dup2(p1[0],0)<0){
					fprintf(stderr,"%s: Can't redirect stdin to read side of 1st pipe: %s\n",
							argv[0],strerror(errno));
					return -1;		
				}
				if(dup2(p2[1],1)<0){
					fprintf(stderr,"%s: Can't redirect stdout to writing side of 2nd pipe: %s\n",
						    argv[0],strerror(errno));
					return -1;		
				}
				//Clean up file descriptors
				if (close(fd)<0){
					fprintf(stderr,"%s: Can't close %s: %s\n",argv[0],fn,strerror(errno));
					return -1;	
				}	
				if(close(p1[0])<0||close(p1[1])<0||close(p2[0])<0||close(p2[1])<0){
					fprintf(stderr,"%s: Can't close pipe fd: %s\n",argv[0],strerror(errno));
					return -1;	
				}
				//Handle SIGPIPE
				if(signal(SIGPIPE,SIG_DFL)==SIG_ERR){
					fprintf(stderr,"%s: Error setting disposition of SIGPIPE to SIG_DFL\n",argv[0]);
					return -1;
				}
				//Exec grep
				if (execlp("grep","grep",pattern,NULL)<0){
					fprintf(stderr,"%s: Can't exec grep: %s\n",argv[0],strerror(errno));
					return -1;
				}
				break;
			default: //PARENT PROCESS	
				switch(cpid2=fork()){
					case -1:
						fprintf(stderr,"%s: More fork failed: %s\n",argv[0],strerror(errno));
						exit(-1);
						break;
					case 0:  //CHILD PROCESS (more)
						//Perform  I/O redirection
						if(dup2(p2[0],0)<0){
							fprintf(stderr,"%s: Can't redirect stdin to read side of 2nd pipe:"
									" %s\n",argv[0],strerror(errno));
							return -1;		
						}
						//Clean up file descriptors
						if (close(fd)<0){
							fprintf(stderr,"%s: Can't close %s: %s\n",argv[0],fn,strerror(errno));
							return -1;	
						}	
						if(close(p1[0])<0||close(p1[1])<0||close(p2[0])<0||close(p2[1])<0){
							fprintf(stderr,"%s: Can't close pipe fd: %s\n",argv[0],strerror(errno));
							return -1;	
						}
						//Exec more
						if (execlp("more","more",NULL)<0){
							fprintf(stderr,"%s: Can't exec more: %s\n",argv[0],strerror(errno));
							return -1;
						}
						break;
					default: //PARENT PROCESS
						//Clean up file descriptors
						if(close(p1[0])<0||close(p2[0])<0||close(p2[1])<0){
							fprintf(stderr,"%s: Can't close pipe fd: %s\n",argv[0],strerror(errno));
							return -1;	
						}
						//Read in from infile and write to pipe
						while((r=my_read(fd,buf,BUF_SIZE,fn,argv))!=0){
							if(my_write(p1[1],buf,r,argv)==-1){ //EPIPE
								break;
							}
						}
						// close infile fd and write side of first pipe
						if (close(fd)<0){
							fprintf(stderr,"%s: Can't close %s: %s\n",argv[0],fn,strerror(errno));
							return -1;	
						}	
						if(close(p1[1])<0){
							fprintf(stderr,"%s: Can't close pipe fd: %s\n",argv[0],strerror(errno));
							return -1;	
						}
						//Wait for children processes to return					
                        if(waitpid(cpid1,&status,0)==-1){
                            fprintf(stderr,"%s: grep wait failed: %s\n",argv[0],strerror(errno));
                            return -1;
                        }
						
                        if(waitpid(cpid2,&status,0)==-1){
                            fprintf(stderr,"%s: more wait failed: %s\n",argv[0],strerror(errno));
                            return -1;
                        }
						
						break;
				}					
				break;
		}		
	}			
	
	return 0;
}

// INT_HANDLER - A signal handler for SIGINT
void int_handler(int sn){
	fprintf(stderr, "Number of fully processed files: %d\nNumber of bytes processed: %d\n",
			n_files,n_bytes);
	exit(-1);
}

// MY_READ - A wrapper to read() that includes error reporting and handling
int my_read(int fd, void *buf, size_t count, char *in, char **argv){
	int retval;
	if((retval=read(fd,buf,count))<0){
		fprintf(stderr,"%s: Can't read from file %s: %s\n",argv[0],in,strerror(errno));exit(-1);
	}		
	return retval;
}

// MY_WRITE - A wrapper to write(), handles short writes and error reporting
int my_write(int fd, const void *buf, size_t count, char **argv){
	int w=0;
	while(w<count){ 
		if((w=write(fd,buf,count))<=0){
			if(errno==EPIPE){// signals user exitting more causing broken pipe
				return -1;
			}
			else{
				fprintf(stderr,"%s: Error writing to  write side of 1st pipe: %s\n",
						argv[0],strerror(errno));
				exit(-1);
			}	
		}	
		count-=w; buf+=w; n_bytes+=w;
	}
	return 0;
}		