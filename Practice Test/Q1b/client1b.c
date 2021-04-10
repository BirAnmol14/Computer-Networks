//BASIC HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
//SOCKET HEADERS
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
//FILE 
#include <fcntl.h>
//MISC
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <time.h>


#define FILENAME "input.txt"
#define ADDR "127.0.0.1"
#define PORT 10345
#define TIMEOUT 2
#define P_SIZE 100

typedef struct pkt{
	int  size;
	int seq_no;
	int channel;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[P_SIZE-3*sizeof(int)-2*sizeof(char)];
}PKT;
PKT curr;
void die(char * c){
		perror(c);
		exit(1);
}
void delay(int ms)
{
    clock_t start_time = clock(); 
    while (clock() < start_time + ms);
}

void printStatus(int type, PKT *p){
	if(type == 0){
			//SENT PKT
			printf("SENT PKT: Seq. No. = %d,Packet Size= %ld ,Payload Size = %d,CH = %d\n",ntohl(p->seq_no),sizeof(PKT),ntohl(p->size), ntohl(p->channel));
			return;
	}
	if(type == 1){
			//RE_TRANSMIT PKT
			printf("RE_TRANSMIT PKT: Seq. No. = %d,Packet Size= %ld ,Payload Size = %d, CH = %d\n",ntohl(p->seq_no),sizeof(PKT),ntohl(p->size),ntohl(p->channel));
			return;
	}
	if(type == 2){
			//ACK PKT
			printf("ACK PKT: Seq. No. = %d, CH = %d\n",ntohl(p->seq_no),ntohl(p->channel));
			return;
	}
}
void SEND(int sock,PKT * p,int PACKET_SIZE,int type){
	int r;
	if(type == 0){
		//send data
		usleep(100*1000);
		r = send(sock,p,sizeof(PKT),0);
		if(r<0){die("SEND Error");}
		printStatus(type,p);
		usleep(100*1000);
	}
}
int main(){
	int PACKET_SIZE = P_SIZE;
	
	int headerSize =  sizeof(curr.size)+sizeof(curr.seq_no)+ sizeof(curr.channel)+sizeof(curr.lst_pkt)+sizeof(curr.TYPE);
	int payloadSize = PACKET_SIZE - headerSize;
	
	if(payloadSize<=0){
		die("Invalid Packet Size");
	}
	
	memset(curr.payload,'\0',payloadSize);
	
	//create 2 TCP socket
	int sock0 = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sock0<0){die("Socket could not be created");}
	int sock1 = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sock1<0){die("Socket could not be created");}
	
	//Set server addr
	struct sockaddr_in serv;
	serv.sin_family =AF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = inet_addr(ADDR);
	
	//Connect to server
	int r = connect(sock0,(struct sockaddr *)&serv,sizeof(serv));
	if(r<0){die("Connect error");}
	sleep(1);
	r = connect(sock1,(struct sockaddr *)&serv,sizeof(serv));
	if(r<0){die("Connect error");}
	sleep(1);
	
	//Open file
	int fd = open(FILENAME,O_RDONLY);
	if(fd<0){die("Unable to read file");}
	
	int n;
	int seq = 0;
	int fdone = 0;
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(sock0,&rset);
	FD_SET(sock1,&rset);
	int max = sock0>sock1?sock0:sock1;
	for(int i=0;i<2;i++){
		int sock;
		if(i==0){
			
			sock = sock0;
			curr.channel = htonl(0);
		}else{
			sock = sock1;
			curr.channel = htonl(1);
		}
		if((n=read(fd,curr.payload,payloadSize))>=0){
			curr.seq_no = htonl(seq);
			curr.size = htonl(n);
			curr.TYPE = 'D';
			if(n==0){
				//EOF
				curr.lst_pkt = '1';
				fdone = 1;
			}else{
				curr.lst_pkt ='0';
			}
			SEND(sock,&curr,PACKET_SIZE,0);
			seq+=sizeof(PKT);
		}else{
			die("Send Error");
		}
	}
	int isPending0=1;
	int isPending1=1;
	int done = 0;
	while(1){
			if(done && !isPending0 && !isPending1){
				break;
			}
			int max = -1;
			if(isPending0){
				FD_SET(sock0,&rset);
				max = max>sock0?max:sock0;
			}
			if(isPending1){
				FD_SET(sock1,&rset);
				max = max>sock1?max:sock1;
			}
			//recv ACK
			select(max+1,&rset,NULL,NULL,NULL);
			if(FD_ISSET(sock0,&rset) ){
				int sock = sock0;
				//ACK on sock0
				PKT rcv;
				PKT curr;
				r = recv(sock,&rcv,sizeof(rcv),0);
				if(r<0){
					die("Recv error");
				}
				if(r==0){
					puts("Server terminated Connection");
					close(sock0);
				}
				if(rcv.TYPE == 'A'){
						isPending0=0;
						printStatus(2,&rcv);
						if(rcv.lst_pkt == '1'){
							done = 1;
							FD_CLR(sock0,&rset);
							continue;
						}
						if(!done &&(n=read(fd,curr.payload,payloadSize))>=0){
							curr.seq_no = htonl(seq);
							curr.size = htonl(n);
							curr.channel = htonl(0);
							curr.TYPE = 'D';
							if(n==0){
								//EOF
								curr.lst_pkt = '1';
								fdone = 1;
							}else{
								curr.lst_pkt ='0';
							}
							SEND(sock,&curr,PACKET_SIZE,0);
							isPending0 = 1;
							seq+=sizeof(PKT);
						}else{
							die("Send Error");
						}
				}else{
					die("SERVER CORRUPT RESPONSE");
				}
			}
			if(FD_ISSET(sock1,&rset) ){
				int sock = sock1;
				//ACK on sock1
				PKT rcv;
				PKT curr;
				r = recv(sock,&rcv,sizeof(rcv),0);
				if(r<0){
					die("Recv error");
				}
				if(r==0){
					puts("Server terminated Connection");
					close(sock);
				}
				if(rcv.TYPE == 'A'){
						isPending1=0;
						printStatus(2,&rcv);
						if(rcv.lst_pkt == '1'){
							done = 1;
							FD_CLR(sock1,&rset);
							continue;
						}
						if(!done && (n=read(fd,curr.payload,payloadSize))>=0){
							curr.seq_no = htonl(seq);
							curr.size = htonl(n);
							curr.channel = htonl(1);
							curr.TYPE = 'D';
							if(n==0){
								//EOF
								curr.lst_pkt = '1';
								fdone = 1;
							}else{
								curr.lst_pkt ='0';
							}
							SEND(sock,&curr,PACKET_SIZE,0);
							isPending1 = 1;
							seq+=sizeof(PKT);
						}else{
							die("Send Error");
						}
				}else{
					die("SERVER CORRUPT RESPONSE");
				}
			}
	}
	close(fd);
	close(sock0);
	close(sock1);
}
