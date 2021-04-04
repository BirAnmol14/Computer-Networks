#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#define SERVER_PORT 12345
#define MAX_QUEUE 5
#define BUFSIZE 32
void handle(int signo){
	if(signo==SIGCHLD){
		wait(NULL);
	}
}
int main(){
	signal(SIGCHLD,handle);
	//1. create socket
	int server_sock=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);//create socket
	if(server_sock<0){printf("socket err");exit(0);}
	printf("Server welcome socket created\n");

	//2. create server addr
	struct sockaddr_in server,client;
	server.sin_family=AF_INET;
	server.sin_port=htons(SERVER_PORT);
	//server.sin_addr.s_addr=inet_addr("127.0.0.1"); for local host
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	printf("Server Addr ready\n");

	//3. bind port 
	int t = bind(server_sock,(struct sockaddr*)&server,sizeof(server));
	if(t<0){
		printf("Error in binding\n");exit(0);
	}
	printf("Socket binding successful\n");

	//4. Listen or be passive active for connections
	t= listen(server_sock,MAX_QUEUE);
	if(t<0){printf("Listen error\n");exit(0);}
	printf("Listening for connections on: %d\n",SERVER_PORT);
	while(1)
	{
		char msg[BUFSIZE];
		int clientLength = sizeof(client);//create a subsocket specific to client for active communication
		int client_sock = accept(server_sock,(struct sockaddr *)&client,&clientLength); // brings back client addr in the client struct you provide
		if(clientLength<0){printf("Error in accepting requests\n");exit(0);}
		else{	
			int pid = fork();
			if(pid==0){
				close(server_sock);//child doesn't need this copy of descriptor
				printf("Client IP: %s\n",inet_ntoa(client.sin_addr));
				printf("Client PORT: %hu\n",ntohs(client.sin_port));
				//receive request
				t = recv(client_sock,msg,BUFSIZE,0);
				if(t<0){printf("Reveive error\n");exit(0);}
				msg[t]='\0';
				printf("%s\n",msg);
				puts("Enter message that you want to send to client: ");
				scanf("%[^\n]",msg);getchar();
				int bytesSent = send(client_sock,msg,strlen(msg),0); 
				if (bytesSent != strlen(msg)){ printf ("Error while sending message to client"); exit(0);}
				//6. terminate 
				close(client_sock);
				exit(0);
			}else{
				close(client_sock);//parent does not need this copy of descriptor
			}
		}
	}
	close(server_sock);
}
