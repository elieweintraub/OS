/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #1: copycat.c                                */
/*                                                                           */
/* copycat-concatenate and copy files                                        */
/*                                                                           */
/* USAGE:                                                                    */
/*		copycat  [-b ####]  [-o outfile] [infile1...infile2...]              */ 
/*                                                                           */ 
/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STDIN 0
#define STDOUT 1
#define BUF_SIZE 4096

#define UNRECOGNIZED_FLG_MSG(x) \
fprintf(stderr, "copycat: Invalid option '%s'\n\
Try 'copycat -h' for more information.\n",(x)); \
exit(-1);

#define USAGE_ERR_MSG \
fprintf(stderr, "copycat: Improper usage detected\n\
Try 'copycat -h' for more information.\n");\
exit(-1);

#define USAGE \
fprintf(stderr,"USAGE: copycat  [-b ####]  [-o outfile]  \
[infile1...infile2...]\n");\
 exit(0);
     	 			
// PARSE_ARGS - A function to parse and handle the command line arguments
int parse_args(int argc,char *argv[],int *BUF_SZ,char **outfile,int *infiles);
// MY_OPEN - A wrapper to open() that includes error reporting and handling
int my_open(char *pathname, char *mode); 
// MY_CLOSE - A wrapper to close() that includes error reporting and handling
int my_close( int fd, char *pathname);
// MY_READ - A wrapper to read() that includes error reporting and handling
int my_read(int fd, void *buf, size_t count, char *infile); 
// MY_WRITE - A wrapper to write(), handles short writes and error reporting
void my_write(int fd, const void *buf, size_t count, char *outfile); 
// MY_MALLOC - A wrapper to malloc() that includes error reporting and handling
void *my_malloc(size_t size);

int main(int argc, char *argv[]){
	int buf_size=BUF_SIZE; // default read/write buffer size
	int out_fd=STDOUT;     // default output file descriptor
	
	int in_fd;             // input file fd
	int n_infiles;         // number of input files
	int infiles[argc];     // array of command line arg indices for the infiles 
	char *buf;             // read/write buffer
	char *infile=NULL;     // infile name
	char *outfile=NULL;    // outfile name
	int n,r;
	
	// Parse command line arguments 
	n_infiles=parse_args(argc,argv,&buf_size,&outfile,infiles);
	
	// Allocate read/write buffer of size buf_size
	buf=(char*)my_malloc(buf_size);

	// Open outfile if necessary
	if (outfile==NULL){ // outfile is STDOUT
		outfile="stdout";
	}
	else{
		out_fd=my_open(outfile,"w");
	}
	// For each infile...
	for(n=0;n<n_infiles;n++){ 
	
		// Open infile if necessary
		if (infiles[n]==-1 || argv[infiles[n]][0]=='-'){ //infile is STDIN
			infile="stdin";
			in_fd=STDIN;
		}	
		else{
			infile=argv[infiles[n]];
			in_fd=my_open(infile,"r");	
		}		
		// Read infile and write to outfile
		while((r=my_read(in_fd,buf,buf_size,infile))!=0){
			my_write(out_fd,buf,r,outfile);
		}
		//Close infile if necessary
		if(in_fd!=STDIN){
			my_close(in_fd,infile);
		}	
	}				
	// Close outfile if necessary
	if(out_fd!=STDOUT){
		my_close(out_fd,outfile);
	}
	// Free buffer
	free(buf);	
	
	return 0;
}

// PARSE_ARGS - Parses the command line arguments, checking for optional flags.
// and setting the corresponding variables.
// Fills an array with the indices into argv[] containing infile names.
// Returns the number of infiles (returning 1 and filling the array with -1 if 
// no infiles are provided, signifying STDIN)
int parse_args(int argc,char *argv[],int *BUF_SZ,char **outfile,int *infiles){
	int arg_n, n_infiles=0;
	char *arg=NULL;
	int i;
	
	for (arg_n=1;arg_n<argc;arg_n++){
		arg=argv[arg_n];
		if (arg[0]=='-' && strlen(arg)!=1){ // If arg has the form of a flag...
			if (arg_n==argc-1 && arg[1]!='h'){ // flag is the last argument 
				USAGE_ERR_MSG;
			}	
			if (strlen(arg)!=2){ // flag has wrong length
				UNRECOGNIZED_FLG_MSG(arg);
			}	
			switch(arg[1]){
				case 'b':
					arg=argv[++arg_n];
					for (i=0;i<strlen(arg);i++){
						if(!isdigit(arg[i])){
							USAGE_ERR_MSG;
						}
					}	
					*BUF_SZ=atoi(arg);
					break;
				case 'o':
					*outfile=argv[++arg_n];
					break;
				case 'h':
					USAGE;
					break;
				default:
					UNRECOGNIZED_FLG_MSG(arg);
			}	
		}
		else// arg is an infile so store its index in the array infiles[]
			infiles[n_infiles++]=arg_n;
	}
	if (n_infiles==0){ // no infiles supplied so use -1 to indicate STDIN
		infiles[n_infiles++]=-1;
	}
	return n_infiles;	
}	

// MY_OPEN - A wrapper to open() that includes error reporting and handling
int my_open(char *pathname, char *mode){
	int fd;
	if (strcmp(mode, "r")==0){ // open file for reading
		if((fd=open(pathname,O_RDONLY))<0){
			fprintf(stderr,"copycat: Can't open file %s for reading: %s\n",
			               pathname,strerror(errno));
			exit(-1);
		}
	}	
	else if (strcmp(mode, "w")==0){ // open file for writing
		if((fd=open(pathname,O_WRONLY|O_CREAT|O_TRUNC,0777))<0){
			fprintf(stderr,"copycat: Can't open file %s for writing: %s\n",
			               pathname,strerror(errno));
			exit(-1);
		}
	}
	else{ 
		fprintf(stderr,"my_open: invalid mode: \"%s\". "
          		       "Expected \"r\" or \"w\" \n", mode);
		exit(-1);
	}		
	return fd;	
} 

// MY_CLOSE - A wrapper to close() that includes error reporting and handling
int my_close( int fd, char *pathname){
	int retval;
	if((retval=close(fd))<0){
		fprintf(stderr,"copycat: Can't close file %s: %s\n",
          		       pathname,strerror(errno));
		exit(-1);
	}
	return retval;			
}

// MY_READ - A wrapper to read() that includes error reporting and handling
int my_read(int fd, void *buf, size_t count, char *infile){
	int retval;
	if((retval=read(fd,buf,count))<0){
		fprintf(stderr,"copycat: Can't read from file %s: %s\n",
          		       infile,strerror(errno));
		exit(-1);
	}		
	return retval;
}

// MY_WRITE - A wrapper to write(), handles short writes and error reporting
void my_write(int fd, const void *buf, size_t count, char *outfile){
	int w=0;
	while(w<count){ 
		if((w=write(fd,buf,count))<=0){
			fprintf(stderr,"copycat: Can't write to file %s: %s\n",
						   outfile,strerror(errno));
			exit(-1);
		}	
		count-=w;
		buf+=w;
	}
}

// MY_MALLOC - A wrapper to malloc() that includes error reporting and handling
void *my_malloc(size_t size){
	void *buf;
	buf=malloc(size);
	if (buf==NULL){
		fprintf(stderr,"copycat: malloc() was unable to allocate"
                       "the requested memory\n");
		exit(-1);
	}		
	return buf;
}			