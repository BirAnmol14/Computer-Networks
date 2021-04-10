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
void SEND(int sock,PKT * p){
	int r;
	//send data
	usleep(100*1000);
	r = send(sock,p,sizeof(PKT),0);
	if(r<0){die("SEND Error");}
	printStatus(0,p);
	usleep(100*1000);
	
}
int main(){
	int PACKET_SIZE = P_SIZE;
	PKT curr;
	int headerSize =  sizeof(curr.size)+sizeof(curr.seq_no)+ sizeof(curr.channel)+sizeof(curr.lst_pkt)+sizeof(curr.TYPE);
	int payloadSize = PACKET_SIZE - headerSize;
	if(payloadSize<=0){
		die("Invalid Packet Size");
	}
	
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
	int pending[2];
	pending[0]=0;
	pending[1]=0;
	PKT rcv[2];
	int n;
	int seq = 0;
	int fdone = 0;
	for(int i=0;i<2;i++){
		int sock;
		if(i==0){
			sock = sock0;
		}else{
			sock = sock1;
		}
		PKT p;
		memset(p.payload,'\0',payloadSize);
		if((n=read(fd,p.payload,payloadSize))>0){
			p.seq_no = htonl(seq);
			p.size = htonl(n);
			p.channel = htonl(i);
			p.TYPE = 'D';
			p.lst_pkt = '0';
			SEND(sock,&p);
			seq += sizeof(PKT);
			pending[i] = 1;
		}else if(n==0){
			p.seq_no = htonl(seq);
			p.size = htonl(n);
			p.channel = htonl(i);
			p.TYPE = 'D';
			p.lst_pkt = '1';
			SEND(sock,&p);
			seq += sizeof(PKT);
			pending[i] = 1;
			fdone = 1;
		}else{
			die("file read error");
		}
	}
	fd_set rset;
	FD_ZERO(&rset);
	FD_SET(sock0,&rset);
	FD_SET(sock1,&rset);
	int max = sock0>sock1?sock0:sock1;
	int done = 0;
	int left = 2;
	while(1){
			if(done && fdone){
					break;
			}
			max = -1;
			if(pending[0]){
				//Ack pending
				FD_SET(sock0,&rset);
				if(max<sock0)max=sock0;
			}
			if(pending[1]){
				FD_SET(sock1,&rset);
				if(max<sock1)max=sock1;
			}
			select(max+1,&rset,NULL,NULL,NULL);
			for(int i=0;i<2;i++){
				int sock;
				if(i==0){
					sock = sock0;
				}else{
					sock = sock1;
				}		
				if(FD_ISSET(sock,&rset)){
					if((n=recv(sock,&rcv[i],sizeof(PKT),0))>0){
						printStatus(2,&rcv[i]);
						pending[i] = 0;
						if(rcv[i].lst_pkt=='1'){
							//last ack received
							done = 1;
							FD_CLR(sock,&rset);
						}
						left--;
					}else if(n==0){
						close(sock);
					}else{
						die("Recv Error");
					}						
				}
			}
			for(int i=0;i<2;i++){
				if(fdone || done){
						break;
				}
				if(pending[i]){
					continue;
				}
				int sock;
				if(i==0){
					sock = sock0;
				}else{
					sock = sock1;
				}
				PKT p;
				memset(p.payload,'\0',payloadSize);
				if((n=read(fd,p.payload,payloadSize))>0){
					p.seq_no = htonl(seq);
					p.size = htonl(n);
					p.channel = htonl(i);
					p.TYPE = 'D';
					p.lst_pkt = '0';
					SEND(sock,&p);
					seq += sizeof(PKT);
					pending[i] = 1;
				}else if(n==0){
					p.seq_no = htonl(seq);
					p.size = htonl(n);
					p.channel = htonl(i);
					p.TYPE = 'D';
					p.lst_pkt = '1';
					SEND(sock,&p);
					seq += sizeof(PKT);
					pending[i] = 1;
					fdone = 1;
				}else{
					die("file read error");
				}
			}
	}
	close(fd);
	close(sock0);
	close(sock1);
}
