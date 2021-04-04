#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#define Buf 512
#define PORT 8882
typedef struct packet1{
	int seq_no;
}AckPacket;
typedef struct packet2{
	int seq_no;
	char data[Buf];
}DataPacket;
typedef struct sockaddr SA;
typedef struct sockaddr_in SA_in;
int getRand(){
	srand(time(0));
	return rand()%5+1;
}
void die(char *s){
	perror(s);
printf("Errno: %d\n",errno);
exit(1);
}
int end=0;
void testEnd(char *s){
	if(strcmp(s,"exit")==0){
		end=1;	
	}
}
int main(){
	int len;
	SA_in server,client;
	char message[Buf];
	char buf[Buf];
	DataPacket recv;
	AckPacket ack;
	int sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock==-1){die("Socket");}

	server.sin_family=AF_INET;
	server.sin_addr.s_addr=htonl(INADDR_ANY);
	server.sin_port=htons(PORT);
	if(bind(sock,(SA*)&server,sizeof(server))<0){
		die("bind()");
	}

	int state=0;
	while(1){
		switch(state){
			case -1: close(sock);exit(0);
			case 0: printf("Waiting for Message(seq0)\n");
				int recvlen=recvfrom(sock,&recv,sizeof(recv),0,(SA*)&client,&len);
				if(recvlen==-1){die("recvfrom()");}
				if(getRand()==1){//simulate packet loss
					printf("Packet Loss\n");
					break;
				}
				if(recv.seq_no==0){
				printf("Received Packet(%d): %s\n",recv.seq_no,recv.data);
				testEnd(recv.data);
				ack.seq_no=0;
				if(sendto(sock,&ack,sizeof(ack),0,(SA*)&client,len)==-1){die("sendto()");}
				if(end==1){state=-1;break;}
				state=1;break;
				}else if(recv.seq_no==1){
					ack.seq_no=1;
					if(sendto(sock,&ack,sizeof(ack),0,(SA*)&client,len)<0){die("sendto()");}
					state=0;break;
				}
			case 1:printf("Waiting for Message(seq1)\n");fflush(stdout);
				if(recvfrom(sock,&recv,sizeof(recv),0,(SA*)&client,&len)==-1)
				{	die("recvfrom()");}
			       if(getRand()==1){
				       printf("Packet Loss\n");
					break;//packet loss
				}
			       if(recv.seq_no==1){
					printf("Packet(%d): %s\n",recv.seq_no,recv.data);
					testEnd(recv.data);
					ack.seq_no=1;
					if(sendto(sock,&ack,sizeof(ack),0,(SA*)&client,len)==-1){
						die("sendto()");
					}
					if(end==1){state=-1;break;}
					state=0;
					break;
				}else if(recv.seq_no==0){
					ack.seq_no=0;
					if(sendto(sock,&ack,sizeof(ack),0,(SA*)&client,len)<0){die("sendto()");}
					state=1;break;
				}	
		}
	}
	close(sock);
	return 0;
}
