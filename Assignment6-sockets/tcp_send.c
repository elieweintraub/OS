/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #6: tcp_send.c                               */
/*                                                                           */
/* tcp_send- creates a unidirectional pipe to send data across a TCP/IP      */
/*           network                                                         */
/*                                                                           */
/*  USAGE:                                                                   */
/*		tcp_send hostname port <input_stream                                 */ 
/*                                                                           */ 
/*****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STDIN 0
#define BUF_SIZE 4096
#define TRUE 1
#define TIMEOUT 30

int main(int argc, char *argv[]){
	struct hostent *he;
	struct sockaddr_in sin;
	struct linger so_linger;
	struct timeval start,end;
	const char *nm;
	char buf[BUF_SIZE],*writebuf;
	unsigned short port;
	int s,r,w,n_bytes=0;
	double start_time,end_time,tr;
	
	// Parse command line arguments
	if(argc!=3){
		fprintf(stderr,"%s: Improper usage!\nUSAGE: %s hostname port <input_stream\n",
				argv[0],argv[0]);
		return-1;
	}
	nm=argv[1];
	port=atoi(argv[2]);
	
	//Set up struct sockaddr_in
	sin.sin_family=AF_INET;
	sin.sin_port=htons(port);	
	if((sin.sin_addr.s_addr=inet_addr(nm))== -1){
		if(!(he=gethostbyname(nm))){
			fprintf(stderr,"%s: Unknown host: %s",argv[0],nm);herror(" "); return -1;
		}
		memcpy(&sin.sin_addr.s_addr,he->h_addr_list[0],sizeof sin.sin_addr.s_addr);
	}
	
	//Establish TCP connection (create local socket and connect to remote socket address)
	if((s=socket(AF_INET,SOCK_STREAM,0))<0){
		fprintf(stderr,"%s: Can't open socket: %s\n",argv[0],strerror(errno));return -1;
	}
	so_linger.l_onoff = TRUE;  
	so_linger.l_linger = TIMEOUT;
	if(setsockopt(s,SOL_SOCKET,SO_LINGER,&so_linger,sizeof so_linger)<0){
		fprintf(stderr,"%s: Error using setsockopt(2) to set SO_LINGER: %s\n",
				argv[0],strerror(errno));
		return -1;
	}	
	if(connect(s,(struct sockaddr *)&sin,sizeof sin)<0){
		fprintf(stderr,"%s: Can't connect socket to host: %s\n",argv[0],strerror(errno));return -1;
	}
	fprintf(stderr,"Socket connected...\n");
		
	//Read data and relay to the remote socket
	gettimeofday(&start,NULL);
	while((r=read(STDIN,buf,BUF_SIZE))!=0){
		if(r<0){
			fprintf(stderr,"%s: Can't read from input: %s\n",argv[0],strerror(errno));
			return -1;
		}
		writebuf=buf;	
		for(w=0;w<r;r-=w,writebuf+=w,n_bytes+=w){ 
			if((w=write(s,writebuf,r))<=0){
				fprintf(stderr,"%s: Can't write to socket: %s\n",argv[0],strerror(errno));return -1;
			}		
		}
	}	
	
	//Clean up and reporting
	if(close(s)<0){
		fprintf(stderr,"%s: Error closing socket: %s\n",argv[0],strerror(errno));return -1;
	}
	gettimeofday(&end,NULL);
	end_time=end.tv_sec+(double)end.tv_usec/1000000;
	start_time=start.tv_sec+(double)start.tv_usec/1000000;
	tr=n_bytes/((end_time-start_time)*1024*1024); //transfer rate in MB/s
	fprintf(stderr,"\nNumber of bytes sent: %d\n\nTransfer Rate: %.6f MB/s\n\n",n_bytes,tr);
		
	return 0;
}