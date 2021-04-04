#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUFSIZE 32
#define SERVER_PORT 12345
int main(){
	//1. create socket
	int sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);//tcp socket
	if(sock<0){printf("socket creation error\n");exit(0);}
	printf("Client Successfully Created Socket\n");
	//2. set server info
	struct sockaddr_in serverAddr;
	serverAddr.sin_family=AF_INET;//address family is set to IP
	serverAddr.sin_port = htons(SERVER_PORT);//server port
	//serverAddr.sin_addr.s_addr=inet_addr("127.0.0.1");//localhost ip by default
	serverAddr.sin_addr.s_addr=inet_addr("192.168.29.231");//server ip
	printf("Address assigned\n");
	//3. establish connection
	int c = connect (sock,(struct sockaddr*)&serverAddr,sizeof(serverAddr));//coneect to server via socket
	if(c<0){printf("Error in connecting...\n");exit(0);}
	printf("connection successfully established\n");

	//4. send data
	puts("Enter data that is to be sent to server: ");
	char buff[BUFSIZE];
	scanf("%[^\n]",buff);
	getchar();
	int byteSent = send(sock,buff,strlen(buff),0);//0 is flag for default behaviour
	if(byteSent!=strlen(buff)){
		puts("Error in send");exit(0);
	}
	printf("Data successfully sent\n");

	//5. Receive bytes
	char buff1[BUFSIZE];
	int byteRecv = recv(sock,buff1,BUFSIZE-1,0);//buff1 should be the address of your buffer, here array so direct name else & name
	if(byteRecv<0){
		puts("Receive Error");
		exit(0);
	}
	buff1[byteRecv]='\0';
	printf("%s\n",buff1);
	close(sock);
}
