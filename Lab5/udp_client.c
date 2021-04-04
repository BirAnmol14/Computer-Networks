#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <time.h>
int end=0;
#define Buf 512
#define PORT 8882
typedef struct packet1{
	int seq_no;
}AckPacket;
typedef struct packet2{
	int seq_no;
	char data[Buf];
}DataPacket;
int gsock;
DataPacket prev;
typedef struct sockaddr SA;
typedef struct sockaddr_in SA_in;
SA_in gserver;
int getRand(){
	srand(time(0));
	return rand()%5+1;
}
void die(char *s){
	perror(s);
printf("Errno: %d\n",errno);
exit(1);
}
void testEnd(char *s){
	if(strcmp(s,"exit")==0){
		end=1;
	}
}
void handle (int signo){
	if(signo == SIGALRM){
		//timeout	
		sendto(gsock,&prev,sizeof(prev),0,(SA*)&gserver,sizeof(gserver));
		alarm(1);//restart timer
		printf("Timeout\n");
	}
}
int main(){
	signal(SIGALRM,handle);
	int len;
	SA_in server;
	char message[Buf];
	char buf[Buf];
	DataPacket send,recv;
	
	int sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock==-1){die("Socket");}
	gsock=sock;
	server.sin_family=AF_INET;
	server.sin_addr.s_addr=inet_addr("127.0.0.1");
	server.sin_port=htons(PORT);
	gserver=server;
	int state=0;
	while(1){
		switch(state){
			case -1:printf("\nbye!!\n");close(sock);exit(0);
			case 0: printf("Enter Message(seq0): ");
				scanf("%[^\n]",send.data);getchar();
				send.seq_no=0;
				prev=send;
				if(sendto(sock,&send,sizeof(send),0,(SA*)&server,sizeof(server))==-1){
					die("sendto()");
				}
				alarm(1);//start timer
				testEnd(send.data);
				state=1;break;
			case 1:if(recvfrom(sock,&recv,sizeof(recv),0,(SA*)&server,&len)==-1){
					die("recvfrom()");
			       }
				   if(errno == EINTR){
						state=1;break;
					}
			       if(getRand()==1){
			       	//ACK LOSS simulation
				printf("ACK loss\n");
				state=1;
				break;
			       }
			       if(recv.seq_no==0){
				       	alarm(0);//stop timer
					printf("received ACK(%d)\n",recv.seq_no);
					if(end){state=-1;break;}
					state=2;
					break;
				}else if(recv.seq_no==1){
					state=1;break;
				}
			case 2:printf("Enter message(seq1): ");
			       scanf("%[^\n]",send.data);getchar();send.seq_no=1;
			       prev=send;
			       if(sendto(sock,&send,sizeof(send),0,(SA*)&server,sizeof(server))==-1){
					die("sendto()");
				  }
			       alarm(1);
			       testEnd(send.data);
			       state=3;break;
			case 3: if(recvfrom(sock,&recv,sizeof(recv),0,(SA*)&server,&len)==-1){
					die("recfrom()");
				}
				if(errno == EINTR){
					state=3;break;
				}
				if(getRand()==1){
					//ACK LOSS
					state=3;
					break;
				}
				if(recv.seq_no==1){
					alarm(0);
					printf("received ACK(%d)\n",recv.seq_no);
					if(end){state=-1;break;}
					state=0;
					break;
				}else if(recv.seq_no==0){
					state=3;break;
				}
		}
	}
	close(sock);
	return 0;
}
