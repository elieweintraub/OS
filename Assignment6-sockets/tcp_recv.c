/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #6: tcp_recv.c                               */
/*                                                                           */
/* tcp_recv- creates a unidirectional pipe to receive data across a TCP/IP   */
/*           network                                                         */
/*                                                                           */
/*  USAGE:                                                                   */
/*		tcp_recv port >output_file                                           */ 
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

#define STDOUT 1
#define BUF_SIZE 4096

int main(int argc, char *argv[]){
	struct hostent *he;
	struct sockaddr_in sin;
	struct timeval start,end;
	char buf[BUF_SIZE],*writebuf;
	unsigned short port;
	int s,s2,r,w,n_bytes=0,len=sizeof sin;
	double start_time,end_time,dr;
	
	// Parse command line arguments
	if(argc!=2){
		fprintf(stderr,"%s: Improper usage!\nUSAGE: %s port >output_file\n",argv[0],argv[0]);
		return-1;
	}
	port=atoi(argv[1]);
	
	//Set up struct sockaddr_in
	sin.sin_family=AF_INET;
	sin.sin_port=htons(port); 
	sin.sin_addr.s_addr=INADDR_ANY;	

	//Establish TCP connection (create local socket, bind, listen, and accept)
	if((s=socket(AF_INET,SOCK_STREAM,0))<0){
		fprintf(stderr,"%s: Can't open socket: %s\n",argv[0],strerror(errno));return -1;
	}
	if(bind(s,(struct sockaddr *)&sin,sizeof sin)<0){
		fprintf(stderr,"%s: Error binding socket: %s\n",argv[0],strerror(errno));
		close(s);
		return -1;
	}	
	if(listen(s,128)<0){
		fprintf(stderr,"%s: Error listening on socket: %s\n",argv[0],strerror(errno));return -1;
	}
	fprintf(stderr,"Socket listening for connection...\n");	
	if ((s2=accept(s,(struct sockaddr *)&sin,&len))<0){
		fprintf(stderr,"%s: Error accepting connection on socket: %s\n",argv[0],strerror(errno));
		return -1;
	}
	fprintf(stderr,"Socket accepted connection...\n");	
	
	//Read received data and copy to stdout
	gettimeofday(&start,NULL);
	while((r=read(s2,buf,BUF_SIZE))!=0){
		if(r<0){
			fprintf(stderr,"%s: Can't read from socket: %s\n",argv[0],strerror(errno));return -1;
		}
		writebuf=buf;	
		for(w=0;w<r;r-=w,writebuf+=w,n_bytes+=w){ 
			if((w=write(STDOUT,writebuf,r))<=0){
				fprintf(stderr,"%s: Can't write to output: %s\n",argv[0],strerror(errno));
				return -1;
			}		
		}
	}	
	
	//Clean up and reporting	
	if(close(s2)<0){
		fprintf(stderr,"%s: Error closing socket: %s\n",argv[0],strerror(errno));return -1;
	}
	gettimeofday(&end,NULL);
	end_time=end.tv_sec+(double)end.tv_usec/1000000;
	start_time=start.tv_sec+(double)start.tv_usec/1000000;
	dr=n_bytes/((end_time-start_time)*1024*1024); //data rate in MB/s
	fprintf(stderr,"\nRemote endpoint Information:\n  IP address : %s\n  port number: %d\n",
			inet_ntoa(sin.sin_addr),ntohs(sin.sin_port));	
	if(he=gethostbyaddr(&sin.sin_addr,sizeof sin.sin_addr,AF_INET)){
		fprintf(stderr,"  hostname   : %s\n",he->h_name);
	}
	else{fprintf(stderr,"  hostname   : unavailable\n");}	
	fprintf(stderr,"\nNumber of bytes received: %d\n\nData Rate: %.6f MB/s\n\n",n_bytes,dr);
	if(close(s)<0){
		fprintf(stderr,"%s: Error closing socket: %s\n",argv[0],strerror(errno));return -1;
	}
	
	return 0;
}