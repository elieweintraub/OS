/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #5: mm2.c                                    */
/*                                                                           */
/*  mm2.c - Answers a series of questions about memory-mapped files.         */
/*                                                                           */ 
/*****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define FILE_SIZE 20

int fd,n_faults=0;
char *addr,**g_argv; 
const char *fname="testfile.txt";
jmp_buf int_jb;

void gen_and_open_testfile(const char *fname,int flag);
int read_testfile(char *buf,int offset, int req);
void file_dump(char *buf, int offset, int req);
void  memory_dump(char *addr, int offset, int req);
void fstat_wrapper(int fd, struct stat *stp);
void *mmap_wrapper(void *addr, size_t length,int prot,int flags,int fd,off_t offset);
void cleanup(int map_size);
void sig_handler(int sn);

int main(int argc, char *argv[]){
	struct stat sb;
	char cmd[100],buf1[100],buf2[100],*fault1,*fault2,c;
	int flag, orig_fsize,final_fsize,sn1,sn2,i;
	const char * flag_str,*update_str;
	g_argv=argv;
	
	// Parse command line arguments
	if(argc!=2 || strlen(argv[1])!=1 || argv[1][0]<'A' || argv[1][0]>'G'){
		fprintf(stderr,"%s: Improper usage. Supply a single question selection [A-G].\n",argv[0]);
		exit(-1);
	}	
	
	switch(argv[1][0]){
		case 'A':	
			printf("A:When one has mapped a file for read-only access, but attempts\n" 
				   "  to write to that mapped area, what signal is generated?\n\n");
			
			printf("Let's find out:\n");		
			
			printf("  Creating a %d byte test file, %s,...\n",FILE_SIZE,fname);
			gen_and_open_testfile(fname,O_RDONLY);
			
			printf("  Mapping the testfile with read only access using mmap()...\n");
			addr=mmap_wrapper(NULL,FILE_SIZE,PROT_READ,MAP_SHARED,fd,0);
			
			for(i=1;i<32;i++){
				if(i==SIGKILL||i==SIGSTOP){continue;}
				if(signal(i,sig_handler)==SIG_ERR){
					fprintf(stderr,"%s: Error setting signal handler for signal # %d\n",argv[0],i);
					return -1;
				}	
			}	
			
			printf("  Attempting to write to the mapped area...\n");	
			strcpy(addr,"Attempting a write.");			
			break;
		
		case 'B':
		case 'C':
			flag=(argv[1][0]=='B')?MAP_SHARED:MAP_PRIVATE;
			flag_str=(argv[1][0]=='B')?"MAP_SHARED":"MAP_PRIVATE";
			
			if(argv[1][0]=='B'){
				printf("B:If one maps a file with MAP_SHARED and then writes to the mapped\n"
					   "  memory, is that update immediately visible when accessing the file\n"
				       "  through the traditional lseek(2)/read(2) system calls?\n\n");
			}else{
				printf("C:If one maps a file with MAP_PRIVATE and then writes to the mapped\n"
				   "  memory, is that update immediately visible when accessing the file\n"
				   "  through the traditional lseek(2)/read(2) system calls?\n\n");
			}	
			
			printf("Let's find out:\n");
			
			printf("  Creating a %d byte test file, %s,...\n",FILE_SIZE,fname);
			gen_and_open_testfile(fname,O_RDWR);
			
			printf("  Checking and displaying initial contents of test file: ");
			read_testfile(buf1,0,FILE_SIZE);
			printf("%s",buf1);
		
			printf("  Mapping the testfile with %s using mmap()...\n",flag_str);
			addr=mmap_wrapper(NULL,FILE_SIZE,PROT_READ|PROT_WRITE,flag,fd,0);
			
			printf("  Writing to the mapped area...\n");
			update_str=	"Update is visible!!\n";
			strcpy(addr,update_str); 
			
			printf("  Checking and displaying current contents of test file: ");
			read_testfile(buf2,0,FILE_SIZE);
			printf("%s",buf2);	
			
			if(!strncmp(buf2,addr,strlen(update_str))){
				printf("\nAnswer: YES, the update is immediately visible.\n\n");
			}
			else{printf("\nAnswer: NO, the update is not immediately visible.\n\n");}
			
			cleanup(FILE_SIZE);		
			break;	

		case 'D':
		case 'E':
			if(argv[1][0]=='E'){
				printf("E:Say a pre-existing file of a certain size which is not an exact\n" 
					   "  multiple of the page size is mapped with MAP_SHARED and read/write\n"
					   "  access, and one writes to the mapped memory just beyond the byte\n"
					   "  corresponding to the last byte of the existing file.After performing\n"
					   "  the aforementioned memory write, one then increased the size of the\n"
					   "  file beyond the written area, without over-writing the intervening\n"
					   "  contents (e.g. by using lseek(2) and then write(2) with a size of 1),\n"
					   "  thus creating a ’hole’ in the file. What happens to the data that had\n"
					   "  been written to this hole previously via the memory-map? Are they\n"
					   "  visible in the file?\n\n");
			}else{		   
				printf("D:Say a pre-existing file of a certain size which is not an exact\n" 
					   "  multiple of the page size is mapped with MAP_SHARED and read/write\n"
					   "  access, and one writes to the mapped memory just beyond the byte\n"
					   "  corresponding to the last byte of the existing file. Does the size\n" 
					   "  of the file through the traditional interface (e.g.stat(2)) change?\n\n");
			}
		
			printf("Let's find out:\n");
			
			printf("  Creating a %d byte test file, %s,...\n",FILE_SIZE,fname);
			gen_and_open_testfile(fname,O_RDWR);
			
			fstat_wrapper(fd,&sb);
			orig_fsize=sb.st_size;	
			printf("  The size of the file as reported by fstat is %d\n",orig_fsize=sb.st_size);
			
			printf("  Mapping the testfile with MAP_SHARED using mmap()...\n");
			addr=mmap_wrapper(NULL,FILE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
			
			printf("  Writing 4 bytes to offset %d...\n",FILE_SIZE);
			strcpy(addr+FILE_SIZE,"XYZ");
			
			fstat_wrapper(fd,&sb);
			final_fsize=sb.st_size;		
			printf("  The new size of the file as reported by fstat is %d\n",final_fsize);
			
			file_dump(buf1,FILE_SIZE,4);
			memory_dump(addr,FILE_SIZE,4);
			if(argv[1][0]=='D'){ 
				if(orig_fsize==final_fsize){
					printf("\nAnswer: NO, the file size does not change.\n");
				}
				else{printf("\nAnswer: YES, the file size does change.\n");}
			}
			else{ //case 'E'
				printf("  About to expand the file by 13 bytes and write(2) to new end...\n");
				if(lseek(fd,FILE_SIZE+12,SEEK_SET)==-1){
					fprintf(stderr,"%s: Error using lseek() on file %s: %s\n",
							argv[0],fname,strerror(errno));
					return -1;
				}
				if(write(fd,"A",1)==-1){
					fprintf(stderr,"%s: Error writing to file %s: %s\n",
					argv[0],fname,strerror(errno));
					return -1;
				}	
				
				file_dump(buf1,FILE_SIZE,16);
				memory_dump(addr,FILE_SIZE,16);
				
				for(i=0;i<4;i++){
					if(addr[FILE_SIZE+i]!=buf1[i]){
						printf("\nAnswer: NO, data written to the hole doesn't remain.\n\n");
						break;
					}
				}	
				if(i==4){printf("\nAnswer: Yes, data written to the hole remains.\n\n");}
			}
			
			cleanup(FILE_SIZE);
			break;

		case 'F':
			printf("F:Let’s say there is an existing small file (say 20 bytes). Can you\n"
			       "  establish a valid mmap region two pages(8192 bytes) long? If so, what\n"
				   "  signal is delivered when attempting to access memory in the second page?\n"
                   "  What about the first page? Explain any differences in these outcomes.\n\n");
			
			printf("Let's find out:\n");
			
			printf("  Creating a %d byte test file, %s,...\n",FILE_SIZE,fname);
			gen_and_open_testfile(fname,O_RDWR);
		
			printf("  Attempting to establish a 2 page mapping to the testfile using mmap()...\n");
			addr=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
			if (addr==MAP_FAILED){
				printf("\nAnswer: NO, you cannot establish a 2 page long mmap region.\n\n");
				cleanup(8192); return 0;	
			}
			
			for(i=1;i<32;i++){
				if(i==SIGKILL||i==SIGSTOP){continue;}
				if(signal(i,sig_handler)==SIG_ERR){
					fprintf(stderr,"%s: Error setting signal handler for signal # %d\n",argv[0],i);
					return -1;
				}	
			}	
			
			if(!(sn2=sigsetjmp(int_jb,1))){
				printf("  Attempting to accesss memory in the second page of the mapped area...\n");	
				c=addr[8000];
			}
			if(!(sn1=sigsetjmp(int_jb,1))){
				printf("  Attempting to accesss memory in the first page of the mapped area...\n");	
				c=addr[0];
			}
			printf("\nAnswer: YES, you can establish a 2 page long mmap region.\n");
			if(sn2){printf("  Upon accessing memory on the second page signal # %d (%s) "
				           "was delivered.\n",sn2,strsignal(sn2));
			}
			else{printf("  Upon accessing memory on the second page no signal was delivered.\n");}
			if(sn1){printf("  Upon accessing memory on the first page signal # %d (%s) "
				           "was delivered.\n\n",sn1,strsignal(sn1));
			}
			else{printf("  Upon accessing memory on the first page no signal was delivered.\n\n");}
			
			//Explanation:  No signal is sent when the process tries to access memory from a region
			//in the first page beyond the end of the file because when a file is smaller than the
			//size of a page the kernel maps in a whole page and zero fills to the page boundary. 
            //However, when the process tries to access memory from the second page the kernel will
			//be unable to satisfy the page fault because the backing store does not exist in the
			//file system. Under this condition the kernel must deliver a SIGBUS to the process.
			
			cleanup(8192);
			break;
		
		case 'G':
			printf("G:Determine the page size of the system experimentally.\n\n");
			
			printf("Let's find out:\n");	
		
			printf("  Establishing address directly after end of heap using sbrk(2)...\n");
			if((addr=sbrk(0))==(void*)-1){
				fprintf(stderr,"%s: Error using sbrk to establish address after end of heap: %s\n",
						argv[0],strerror(errno));
				return -1;		
			}
			printf("  Heap ends at address %p\n",addr-1);
			
			if(signal(SIGSEGV,sig_handler)==SIG_ERR){
				fprintf(stderr,"%s: Error setting signal handler for SIGSEGV\n",argv[0]);return -1;
			}	
			
			for(i=0;n_faults<2;i++){
				if((sigsetjmp(int_jb,1))){ 
					if(n_faults==1){
						fault1=addr+i;
						printf("  Page begins at address %p\n",addr+i);
						printf("  Growing memory region by one byte...\n");
						if(sbrk(1)==(void*)-1){
							fprintf(stderr,"%s: Error using sbrk: to grow heap by one byte: %s\n",
							        argv[0],strerror(errno));
							return -1;		
						}
						continue;
					} 
					else{ //n_faults==2
						fault2=addr+i;
						printf("  Page begins at address %p\n",addr+i);
						continue;
					} 
				}	
				c=addr[i];
			}
			printf("\nAnswer: The page size is %d bytes.\n\n",fault2-fault1);	
			break;
	}
	
	return 0;
}	

// Generates a 20 byte test file, testfile.txt, and opens it in the mode determined by flag 
void gen_and_open_testfile(const char *fname,int flag){
	char cmd[100];
	const char *flag_str=(flag==O_RDONLY)?"reading":"reading and writing";
	sprintf(cmd,"echo \"This is a testfile!\" >%s",fname);
	if(system(cmd)==-1){fprintf(stderr,"mm2: Error using system() to create testfile."); exit(-1);} 
	if((fd=open(fname,flag))<0){
		fprintf(stderr,"mm2: Can't open file %s for %s: %s\n",fname,flag_str,strerror(errno));
		exit(-1);		
	}
}

// Reads from specified offset in testfile into provided buffer. Returns number of bytes read.
// Reads up to req bytes from file and appends '\0' to end so buf should be at least req+1 bytes).
int read_testfile(char *buf,int offset, int req){
	char *buf_addr=buf;
	int r;
	if(lseek(fd,offset,SEEK_SET)==-1){
        fprintf(stderr,"mm2: Error using lseek() on file %s: %s\n",fname,strerror(errno));exit(-1);
    }
	while((r=read(fd,buf_addr,req))!=0){
		if(r<0){
			fprintf(stderr,"mm2: Can't read from file %s: %s\n",fname,strerror(errno));exit(-1);
		}	
		buf_addr+=r;
		req-=r;
	}
	buf_addr[0]='\0'; //add null character to end of buffer so can print file contents.
	return buf_addr-buf;
}

//Wrapper to read_testfile(), prints the returned contents in hex together w/ other filedump info  
void file_dump(char *buf, int offset, int req){
	int ret,n;
	ret=read_testfile(buf,offset,req);
	printf("  File dump starting at offset %d, read req %d, ret %d\n",offset,req,ret);
	for(n=0;n<ret;n++){printf("  <%02X>  ",buf[n]);}
	(n>0)?printf("\n  <EOF>\n"):printf("  <EOF>\n");	
}

//Returns a memory dump of req bytes starting at address addr+offset
void  memory_dump(char *addr, int offset, int req){
	int n;
	printf("  Memory dump starting at offset %d, for %d bytes\n",offset,req);
	for(n=0;n<req;n++){printf("  <%02X>  ",addr[offset+n]);}
	putchar('\n');
}	

// Error handling wrapper to fstat()
void fstat_wrapper(int fd, struct stat *stp){
	if(fstat(fd,stp)<0){
		fprintf(stderr,"mm2: Error using fstat() on file %s: %s\n",fname,strerror(errno));exit(-1);
	}
}	

// Error handling wrapper to mmap()
void *mmap_wrapper(void *addr, size_t length,int prot,int flags,int fd,off_t offset){
	void *retval=mmap(addr,length,prot,flags,fd,offset);
	if (retval==MAP_FAILED){
		fprintf(stderr,"%mm2: Error using mmap() to map file: %s\n",strerror(errno));exit(-1);		
	}
}
 
// Performs basic post program cleanup	
void cleanup(int map_size){
	if (close(fd)<0){
		fprintf(stderr,"mm2: Can't close %s: %s\n",fname,strerror(errno));exit(-1);	
	}
	if(remove(fname)<0){
		fprintf(stderr,"mm2: Can't delete %s: %s\n",fname,strerror(errno));exit(-1);	
	}
	if(munmap(addr,map_size)<0){
		fprintf(stderr,"mm2: Error using munmap(): %s\n",strerror(errno));exit(-1);	
	}
}

// Handles Signals	
void sig_handler(int sn){      
	printf("  Signal # %d (%s) caught!\n",sn,strsignal(sn));
	switch(g_argv[1][0]){
		case 'A':
			printf("\nAnswer: signal # %d (%s) was generated upon attempt to write to "
			       "mapped area.\n\n",sn,strsignal(sn));
			cleanup(FILE_SIZE);
			exit(0);
			break;
		case 'F':
			longjmp(int_jb,sn);
			break;
		case 'G':
			n_faults++;
			longjmp(int_jb,1);
			break;
	}
}
