#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#define BUF 256
#define PORT 12345
void die(char *s){
	perror(s);
	printf("errno: %d\n",errno);
	exit(1);
}
int main(){
	int sock;//socket fd
	int bytesRecv = 0;
	char recvBuf[BUF];
	unsigned char buff_offset[10]; //buffer to send the file offset value
	unsigned char buff_cmd[2];// command 0 => Complete File ; command 1=> partial file
	int offset; // needed in case of checkpointing partial file
	int command; // user enter value
	memset(recvBuf,'0',BUF);
	struct sockaddr_in server;
	if((sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))==-1){
		die("socket err");
	}
	server.sin_family=AF_INET;
	server.sin_port=htons(PORT);
	server.sin_addr.s_addr=inet_addr("127.0.0.1");
	
	if(connect(sock,(struct sockaddr *)&server,sizeof(server))==-1){
		die("connect err");
	}

	FILE * fp;
	fp  = fopen("destination_file.txt","ab");
	if(fp==NULL){die("file err");}

	fseek(fp,0,SEEK_END);
	offset=ftell(fp);
	fclose(fp);

	fp  = fopen("destination_file.txt","ab");
	if(fp==NULL){die("file err");}

	puts("*******************************************");
	puts("(0) To get complete file");
	puts("(1) To specify file offset");
	puts("(2) Calculate file offset from local file");
	puts("Enter Command: ");
	scanf("%d",&command);
	sprintf(buff_cmd,"%d",command);//copy command to buffer string
	write(sock,buff_cmd,2);//send to server the command
	if(command ==1 || command ==2){
		if(command==1){
			printf("Enter File offset: ");
			scanf("%d",&offset);
		}
		sprintf(buff_offset,"%d",offset);
		write(sock,buff_offset,10);//send and write to server the file offset
	}	
	else{
		//command 0 offset not needed
	}
	fseek(fp,offset,SEEK_SET);
	//receive file in chunks of BUF
	while((bytesRecv=read(sock,recvBuf,BUF))>0){
		printf("Bytes Received %d\n",bytesRecv);
		fwrite(recvBuf,1,bytesRecv,fp);
	}
	if(bytesRecv<0){die("File download error");}
	//read return 0 => EOF
	fclose(fp);
	close(sock);
	return 0;
}
