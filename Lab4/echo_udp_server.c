#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#define BUF 256
#define PORT 12345

void die(char * s){
	printf("errono: %d\n",errno);
	puts(s);
	exit(0);
}

int main(){
	//UDP server
	//
	struct sockaddr_in server,client;
	int clilen,sock;
	if((sock=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1){
		die("socket()");
	}
	memset((char *)&server,0,sizeof(server));
	server.sin_family=AF_INET;
	server.sin_port=htons(PORT);
	server.sin_addr.s_addr=htonl(INADDR_ANY);

	if(bind(sock,(struct sockaddr*)&server,sizeof(server))==-1){
		die("bind()");
	}

	while(1){
		puts("Server waiting for data");
		fflush(stdout);
		char buf[BUF];
		memset(buf,'\0',BUF);
		int r=recvfrom(sock,buf,BUF,0,(struct sockaddr*)&client,&clilen);
		if(r==-1){die("recvfrom()");}
		puts("Client sent: ");
		puts(buf);
		r=sendto(sock,buf,r,0,(struct sockaddr*)&client,clilen);
		if(r==-1){die("sendto()");}
	}
	close(sock);
	return 0;
}
