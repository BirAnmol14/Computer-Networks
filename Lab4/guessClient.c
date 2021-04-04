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
		int guess=0;
		puts("Guess a number in range [1-6]: ");
		scanf("%d",&guess);
		int r=sendto(sock,&guess,sizeof(guess),0,(struct sockaddr*) &other, len);
		if(r==-1){die("sendto()");}
		int answer;
		r= recvfrom(sock,&answer,sizeof(answer),0,(struct sockaddr*)&other,&len);
		if(r==-1){die("recvfrom()");}
		if(answer==guess){
			puts("Guess was right");
		}
		else {printf("Oops!, correct answer was: %d\n",answer);}
	
	close(sock);
	return 0;
}
