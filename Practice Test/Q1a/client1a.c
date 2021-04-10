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
#define PORT 12121
#define TIMEOUT 2
#define P_SIZE 100

typedef struct pkt{
	int  size;
	int seq_no;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[P_SIZE-2*sizeof(int)-2*sizeof(char)];
}PKT;
int gsock;
int g_pktSiz;
int gpayLoad;
PKT last;
PKT curr;
void die(char * c){
		perror(c);
		exit(1);
}
void backup(PKT *p1,PKT *p2){
		int payloadSize = gpayLoad;
		p2->size=p1->size;
		p2->seq_no = p1->seq_no;
		p2->lst_pkt = p1->lst_pkt;
		p2->TYPE = p1->TYPE;
		memset(p2->payload,'\0',payloadSize);
		strcpy(p2->payload,p1->payload);
		memset(p1->payload,'\0',payloadSize);
}
void printStatus(int type, PKT *p){
	if(type == 0){
			//SENT PKT
			printf("SENT PKT: Seq. No. = %d,Packet Size= %ld ,Payload Size = %d\n",ntohl(p->seq_no),sizeof(PKT),ntohl(p->size));
			return;
	}
	if(type == 1){
			//RE_TRANSMIT PKT
			printf("RE_TRANSMIT PKT: Seq. No. = %d,Packet Size= %ld ,Payload Size = %d\n",ntohl(p->seq_no),sizeof(PKT),ntohl(p->size));
			return;
	}
	if(type == 2){
			//ACK PKT
			printf("ACK PKT: Seq. No. = %d\n",ntohl(p->seq_no));
			return;
	}
}
void SEND(int sock,int PACKET_SIZE,int type){
	int r;
	if(type == 0){
		backup(&curr,&last);
		//send data
		r = send(sock,&last,sizeof(last),0);
		if(r<0){die("SEND Error");}
		printStatus(type,&last);
	}else if(type == 1){
		//send data
		r = send(sock,&last,sizeof(last),0);
		if(r<0){die("SEND Error");}
		printStatus(type,&last);
	}
}
void handle(int signo){
		if(signo == SIGALRM){
			//TIMEOUT
			alarm(TIMEOUT);
			SEND(gsock,g_pktSiz,1);
		}
}
int main(){
	int PACKET_SIZE = P_SIZE;
	g_pktSiz = PACKET_SIZE;
	int headerSize =  sizeof(curr.size) +  sizeof(curr.seq_no)+ sizeof(curr.lst_pkt)+sizeof(curr.TYPE);
	int payloadSize = PACKET_SIZE - headerSize;
	gpayLoad = payloadSize;
	if(payloadSize<=0){
		die("Invalid Packet Size");
	}
	
	//SET ALARM HANDLER
	struct sigaction act;
	act.sa_handler = handle;
	act.sa_flags = SA_RESTART; // restart the recv call after interrupt handled
	sigaction(SIGALRM,&act,NULL);
	memset(curr.payload,'\0',payloadSize);
	memset(last.payload,'\0',payloadSize);
	//create TCP socket
	int sock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sock<0){die("Socket could not be created");}
	gsock  = sock;
	//Set server addr
	struct sockaddr_in serv;
	serv.sin_family =AF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = inet_addr(ADDR);
	
	//Connect to server
	int r = connect(sock,(struct sockaddr *)&serv,sizeof(serv));
	if(r<0){die("Connect error");}
	sleep(2);
	//Open file
	int fd = open(FILENAME,O_RDONLY);
	if(fd<0){die("Unable to read file");}
	
	int n;
	int seq = 0;
	int done = 0;
	while(!done && (n=read(fd,curr.payload,payloadSize))>=0){
			curr.seq_no = htonl(seq);
			curr.size = htonl(n);
			if(n==0){
				//EOF
				curr.lst_pkt = '1';
				done = 1;
			}else{
				curr.lst_pkt ='0';
			}
			curr.TYPE = 'D';
			SEND(sock,PACKET_SIZE,0);
			//recv ACK
			alarm(TIMEOUT);
			PKT rcv;
			r = recv(sock,&rcv,sizeof(rcv),0);
			if(r<0){
					if(errno == EINTR){
						//alarm went off - auto restart at recv
					}else{
							die("Recv error");
					}
			}
			if(r==0){
				puts("Server terminated Connection");
				break;
			}
			alarm(0);
			if(rcv.TYPE == 'A' && last.seq_no == rcv.seq_no){
					printStatus(2,&rcv);
			}else{
				die("SERVER CORRUPT RESPONSE");
			}
			seq+=sizeof(last);
			usleep(100 * 1000); // 100ms sleep
	}
	if(n<0){die("READ ERROR");}
	close(fd);
	close(sock);
}
