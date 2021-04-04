#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
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

	
		puts("Server Generating new random number in range [1-6]");
		srand(time(0));
		int random = rand()%6+1;
		printf("Generated: %d\n",random);
		fflush(stdout);
		int buf=0;
		int r=recvfrom(sock,&buf,sizeof(buf),0,(struct sockaddr*)&client,&clilen);
		if(r==-1){die("recvfrom()");}
		printf("Client(%s:%d) guessed: %d\n",inet_ntoa(client.sin_addr),ntohs(client.sin_port),buf);
		r=sendto(sock,&random,sizeof(random),0,(struct sockaddr*)&client,clilen);
		if(r==-1){die("sendto()");}

	close(sock);
	return 0;
}
