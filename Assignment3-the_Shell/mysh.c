/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #3: mysh.c                                   */
/*                                                                           */
/*  mysh.c - A simplified UNIX shell supporting interactive                  */
/*           and non-interactive modes.                                      */
/*  USAGE:                                                                   */
/*		   mysh                                                              */
/*         mysh /script                                                      */ 
/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
	
int main(int argc, char *argv[]){
	FILE *fp=stdin;
	char *line = NULL, *token, **args, **redir_ops;
    size_t len = 0;
    ssize_t read;
	pid_t cpid;
	int fd, status, n_args, i, max_args=10, max_redir_ops=3; ;
	int op_len; //length of redirection operator
	mode_t mode;
	struct rusage ru;
	struct timeval start, end, diff;

	//Parse commmand line arguments 
	if ( argc > 2 ){
		fprintf(stderr,"%s: Too many command line arguments!\n",argv[0]); exit(-1);
	}
	if ( argc==2 && !(fp = fopen(argv[1], "r"))){
		fprintf(stderr,"%s: Error opening file %s: %s\n",argv[0],argv[1],strerror(errno));exit(-1);
	}	
	//Allocate buffers to store parsed command
	if(!(args=(char**)malloc(max_args*sizeof(char*)))){
		fprintf(stderr,"%s: Out of memory\n",argv[0]);exit(-1);
	}
	if(!(redir_ops=(char**)malloc(max_redir_ops*sizeof(char*)))){
					fprintf(stderr,"%s: Out of memory\n",argv[0]);exit(-1);
	}	
	//Read in command from command line or script and parse 
	while ( ((fp==stdin)?printf("[mysh]$ "):1) && (read = getline(&line, &len, fp)) != -1){
		if (line[0]=='#') {continue;}
		if (line[strlen(line)-1]=='\n'){line[strlen(line)-1]='\0';}	
		//Collect command		
		args[0]=strtok(line," \t"); 
		if (!args[0]) {continue;}   //for case when user only hits enter
		//Collect arguments
		redir_ops[0]=NULL;          //flush redir_ops buffer at start of each parse 
		for(i=1;token=strtok(NULL," \t");i++) { 
			if ( token[0]=='<' || token[0]=='>' || !strncmp(token,"2>",2)){	
				redir_ops[0]=token; break;
			}	 
			if(i<max_args){
				args[i]=token;
			}
			else{ 
				max_args*=2;
				if(!(args=(char**)realloc(args,max_args*sizeof(char*)))){
					fprintf(stderr,"%s: Out of memory\n",argv[0]);exit(-1);
				}	
				args[i]=token;	
			}
		}
		n_args=i; args[i]=NULL;
		//Collect redirections
		for(i=1;token=strtok(NULL," \t");i++) { 
			if(i<max_redir_ops){
				redir_ops[i]=token;
			}
			else{ 
				max_redir_ops*=2;
				if(!(redir_ops=(char**)realloc(redir_ops,max_redir_ops*sizeof(char*)))){
					fprintf(stderr,"%s: Out of memory\n",argv[0]);exit(-1);
				}
				redir_ops[i]=token;
			}
		}
		redir_ops[i]=NULL; 
		//Report command executed along with its arguments
		fprintf(stderr,"Executing command \"%s\"",args[0]);
		if (args[1]){ //if there are arguments
			fprintf(stderr," with arguments");
			for(i=1;i<n_args-1;i++){fprintf(stderr," \"%s\",",args[i]);}
			fprintf(stderr," \"%s\".\n",args[i]);
		}
		else{fprintf(stderr,"\n");}	
		//Fork 
		switch(cpid=fork()){
			case -1:
				fprintf(stderr,"%s: Fork failed: %s\n",argv[0],strerror(errno)); exit(-1);
				break;
			case 0:  //CHILD PROCESS
    			//Perform  I/O redirection
				for(i=0;redir_ops[i];i++){
					if(redir_ops[i][0]=='<'){ //stdin redirection
						op_len=1; 
						if((fd=open(redir_ops[i]+op_len,O_RDONLY))<0){
							fprintf(stderr,"%s: Can't open file %s for reading: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;		
						}
						if(dup2(fd,0)<0){
							fprintf(stderr,"%s: Can't redirect stdin to %s: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;		
						}		
					}
					else if(redir_ops[i][0]=='>'){ //stdout redirection
						if (redir_ops[i][1] == '>'){op_len=2; mode=O_WRONLY|O_CREAT|O_APPEND;}
						else{op_len=1; mode=O_WRONLY|O_CREAT|O_TRUNC;}
						if((fd=open(redir_ops[i]+op_len,mode,0666))<0){
							fprintf(stderr,"%s: Can't open file %s for writing: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;		
						}
						if(dup2(fd,1)<0){
							fprintf(stderr,"%s: Can't redirect stdout to %s: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;		
						}
					}
					else if(redir_ops[i][0]=='2'){ //stderr redirection
						if(redir_ops[i][2]=='>'){op_len=3; mode=O_WRONLY|O_CREAT|O_APPEND;}
						else{op_len=2; mode=O_WRONLY|O_CREAT|O_TRUNC;}
						if((fd=open(redir_ops[i]+op_len,mode,0666))<0){
							fprintf(stderr,"%s: Can't open file %s for writing: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;		
						}
						if(dup2(fd,2)<0){
							fprintf(stderr,"%s: Can't redirect stderr to %s: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;		
						}
					}	
					else{ 
						fprintf(stderr,"%s: Invalid syntax\n"
						        "Usage:[argument ...] [redirection_operation...]\n",argv[0]);
						return -1;
					}
					//Clean file descriptor environment
					if(close(fd)<0){
							fprintf(stderr,"%s: Can't close file %s: %s\n",
									argv[0],redir_ops[i]+op_len,strerror(errno));
							return -1;	
					}
				}			
				//Exec the program
				if (execvp(args[0],args)<0){
					fprintf(stderr,"%s: Can't exec %s: %s\n",argv[0],args[0],strerror(errno));
					return -1;
				}
				break;
			default: //PARENT PROCESS
				//Wait and calculate  time spent in child process
				gettimeofday(&start,NULL);	
				if( wait4(cpid,&status,0,&ru)==-1){
					fprintf(stderr,"%s: wait failed: %s\n",argv[0],strerror(errno)); exit(-1);
				}
				gettimeofday(&end,NULL);
				if (end.tv_usec < start.tv_usec) {
					diff.tv_sec=end.tv_sec-start.tv_sec-1;
					diff.tv_usec=1000000+end.tv_usec-start.tv_usec;
				} 
				else{
					diff.tv_sec = end.tv_sec-start.tv_sec;
					diff.tv_usec = end.tv_usec-start.tv_usec;
				}
				//Report exit status and resource usage
				if (WIFSIGNALED(status)){
					fprintf(stderr,"Command exited with signal %d, ",WTERMSIG(status));
				}
				else{ // (WIFEXITED(status))
					fprintf(stderr,"Command exited with return code %d, ",WEXITSTATUS(status));
				}
				fprintf(stderr,"consuming\n%ld.%.6d real seconds, %ld.%.6d user, %ld.%.6d "
					    "system.\n", diff.tv_sec, diff.tv_usec, ru.ru_utime.tv_sec,
						ru.ru_utime.tv_usec, ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);		
		}
	}
	//Post-program cleanup 
	if(fp==stdin) {printf("end of file\n");} 
	free(args); free(redir_ops); free(line);	
	return 0;
}