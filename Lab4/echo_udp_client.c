#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#define BUF 256
#define PORT 12345

void die(char * s){
	perror(s);
	exit(1);
}
int main(){
	struct sockaddr_in other;
	int len;
	int sock;
	if((sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1){
		die("socket");
	}
	other.sin_family=AF_INET;
	other.sin_port=htons(PORT);
	other.sin_addr.s_addr = inet_addr("127.0.0.1");
	len=sizeof(other);
	while(1){
		char msg[BUF];
		puts("Enter message you want to send: ");
		scanf("%[^\n]",msg);
		getchar();
		int r=sendto(sock,msg,strlen(msg),0,(struct sockaddr*) &other, len);
		if(r==-1){die("sendto()");}
		memset(msg,'\0',BUF);//empty the buffer
		r= recvfrom(sock,msg,BUF,0,(struct sockaddr*)&other,&len);
		if(r==-1){die("recvfrom()");}
		puts("Server Echoed: ");
		puts(msg);
		if(r==0){
		break;
		}
	}
	close(sock);
	return 0;
}
