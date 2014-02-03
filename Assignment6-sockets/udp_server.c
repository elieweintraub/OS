/*****************************************************************************/
/*  Elie Weintraub                                                           */
/*  OS - Programming Assignment #6: udp_server.c                             */
/*                                                                           */
/* udp_server- a UDP server program which listens for requests on a          */
/*             specified UDP port and returns information to the requester   */
/*                                                                           */
/*  USAGE:                                                                   */
/*		udp_server port                                                      */ 
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

int get_command_output(const char *cmd,char *output_buf);

int main(int argc, char *argv[]){
	struct sockaddr_in sin;
	char buf[BUF_SIZE],output_buf[BUF_SIZE];
	unsigned short port;
	int s,len=sizeof sin;
	
	// Parse command line arguments
	if(argc!=2){
		fprintf(stderr,"%s:Improper usage!\nUSAGE:%s port\n",argv[0],argv[0]);return-1;
	}
	port=atoi(argv[1]);
	
	//Set up struct sockaddr_in
	sin.sin_family=AF_INET;
	sin.sin_port=htons(port); 
	sin.sin_addr.s_addr=INADDR_ANY;	

	//Create local socket and bind
	if((s=socket(AF_INET,SOCK_DGRAM,0))<0){
		fprintf(stderr,"%s:Can't open socket:%s\n",argv[0],strerror(errno));return -1;
	}
	if(bind(s,(struct sockaddr *)&sin,sizeof sin)<0){
		fprintf(stderr,"%s:Error binding socket:%s\n",argv[0],strerror(errno));
		close(s);
		return -1;
	}	
	while(1){// Listen for requests and return information to the requester 
		if(recvfrom(s,buf,BUF_SIZE,0,(struct sockaddr *)&sin,&len)==-1){
			fprintf(stderr,"%s:Error receiving request:%s\n",argv[0],strerror(errno));continue;
		}
		fprintf(stderr,"Received req from %s:%d...\n",inet_ntoa(sin.sin_addr),ntohs(sin.sin_port));
		if(!strcmp(buf,"UPTIME")){
			if(get_command_output("uptime",output_buf)==-1){continue;}
			if(sendto(s,output_buf,strlen(output_buf)+1,0,(struct sockaddr *)&sin,len)==-1){
				fprintf(stderr,"%s:Error sending response:%s\n",argv[0],strerror(errno));continue;
			}
		}
		else if(!strcmp(buf,"DATE")){
			if(get_command_output("date",output_buf)==-1){continue;}
			if(sendto(s,output_buf,strlen(output_buf)+1,0,(struct sockaddr *)&sin,len)==-1){
				fprintf(stderr,"%s:Error sending response:%s\n",argv[0],strerror(errno));continue;
			}
		}
		else{fprintf(stderr,"%s:Unrecognized message!\n",argv[0]);continue;}
		fprintf(stderr,"Sent resp to %s:%d...\n\n",inet_ntoa(sin.sin_addr),ntohs(sin.sin_port));
	}
	
	return 0;
}

//Gets first line of output of cmd and stores in output_buf.
//Returns -1 if there was an error executing the command. 
int get_command_output(const char *cmd,char *output_buf){
	FILE *stream;
	if(!(stream=popen(cmd, "r")) || !(fgets(output_buf,BUF_SIZE,stream))){
		fprintf(stderr,"udp_server:Error executing command:%s\n",strerror(errno));return -1;
	}	
	if(pclose(stream)==-1){fprintf(stderr,"udp_server:Error closing command stream\n");}
	return 0;
}