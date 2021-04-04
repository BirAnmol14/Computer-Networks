#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#define PORT 12345
#define BUF 256
#define MAX 5
void die(char * s){
		perror(s);
		printf("errno: %d\n",errno);
		exit(1);
}
int main(){
	while(1){
		int sockfd;
		struct sockaddr_in server;
		char buf[BUF]={0};
		
		sockfd=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
		if(sockfd==-1){
			die("socket()");
		}
		
		server.sin_family=AF_INET;
		server.sin_port=htons(PORT);
		server.sin_addr.s_addr=htonl(INADDR_ANY);
		
		if(bind(sockfd,(struct sockaddr *)&server,sizeof(server))==-1){
			die("bind()");
		}
			unsigned char offset_buf[10]={'\0'};
			unsigned char cmd_buf[2]={'\0'};
			int offset;
			int command;
			struct sockaddr_in client;int clilen;
			
			printf("Waiting for client to enter command\n");
			//0 full file, 1 partial file from client offset, partial file from  computed offset
						
			while(recvfrom(sockfd,cmd_buf,2,0,(struct sockaddr *)&client,&clilen)==0);
			sscanf(cmd_buf,"%d",&command);
			
			if(command==0){offset=0;}
			else{
				printf("Waiting for offset from client\n");
				while(recvfrom(sockfd,offset_buf,10,0,(struct sockaddr *)&client,&clilen)==0);
				sscanf(offset_buf,"%d",&offset);
			}
			
			FILE * fp =fopen("source_file.txt","rb");
			if(fp==NULL){die("Source File");}
			fseek(fp,offset,SEEK_SET);
			while(1){
				int nread = fread(buf,1,BUF,fp);
				printf("Read %d bytes\n",nread);
				if(nread>0){
						printf("Uploading\n");
						int ret=sendto(sockfd,buf,nread,0,(struct sockaddr *)&client,clilen);
				}
				if(nread<BUF){
						int ret=sendto(sockfd,NULL,0,0,(struct sockaddr *)&client,clilen);
						if(feof(fp)){printf("Upload Complete\n");}
						if(ferror(fp)){printf("File error\n");}
						break;
				}
			}
			fclose(fp);
			close(sockfd);
	}
}
