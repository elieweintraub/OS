/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #6: udp_client.c                             */
/*                                                                           */
/* udp_client- a complimentary UDP client program which sends the request,   */
/*             receives the response and prints it out                       */
/*                                                                           */
/*  USAGE:                                                                   */
/*		udp_client hostname port request_string                              */ 
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

#define BUF_SIZE 256

int main(int argc, char *argv[]){
	struct sockaddr_in sin;
	struct hostent *he;
	const char *nm,*req_str;
	char buf[BUF_SIZE];
	unsigned short port;
	int s,len=sizeof sin;
	
	// Parse command line arguments
	if(argc!=4){
		fprintf(stderr,"%s:Improper usage!\nUSAGE:%s hostname port request_string\n",
				argv[0],argv[0]);
		return-1;
	}
	if(strcmp(argv[3],"UPTIME") && strcmp(argv[3],"DATE")){
		fprintf(stderr,"%s:Improper usage! request_string must be either 'UPTIME' or 'DATE'\n",
				argv[0]);
		return -1;
	}	
	nm=argv[1];
	port=atoi(argv[2]);
	req_str=argv[3];
	
	//Set up struct sockaddr_in
	sin.sin_family=AF_INET;
	sin.sin_port=htons(port);	
	if((sin.sin_addr.s_addr=inet_addr(nm))== -1){
		if(!(he=gethostbyname(nm))){
			fprintf(stderr,"%s: Unknown host: %s",argv[0],nm);herror(" "); return -1;
		}
		memcpy(&sin.sin_addr.s_addr,he->h_addr_list[0],sizeof sin.sin_addr.s_addr);
	}
	
	//Ceate local socket 
	if((s=socket(AF_INET,SOCK_DGRAM,0))<0){
		fprintf(stderr,"%s: Can't open socket: %s\n",argv[0],strerror(errno));return -1;
	}
		
	//Send request and receive and print response
	if(sendto(s,req_str,strlen(req_str)+1,0,(struct sockaddr *)&sin,len)==-1){
		fprintf(stderr,"%s:Error sending request:%s\n",argv[0],strerror(errno));return -1;
	}	
	fprintf(stderr,"Sent req to %s:%d...\n",inet_ntoa(sin.sin_addr),ntohs(sin.sin_port));
	if(recvfrom(s,buf,BUF_SIZE,0,(struct sockaddr *)&sin,&len)==-1){
			fprintf(stderr,"%s:Error receiving response:%s\n",argv[0],strerror(errno));return -1;
		}
	fprintf(stderr,"Received resp from %s:%d...\n",inet_ntoa(sin.sin_addr),ntohs(sin.sin_port));
	printf("Response: %s\n",buf);
			
	//Clean up 
	if(close(s)<0){
		fprintf(stderr,"%s: Error closing socket: %s\n",argv[0],strerror(errno));return -1;
	}
		
	return 0;
}