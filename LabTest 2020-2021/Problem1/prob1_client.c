/*
Name: BIR ANMOL SINGH
ID: 2018A7PS0261P
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <time.h>


#define FILENAME "input.txt"
#define ADDR "127.0.0.1"
#define PORT 11223
#define TIMEOUT 2
#define MAXLINE 256 //asume that each line has a max size to be UDP safe

typedef struct pkt{
	int  size;
	int seq_no;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[MAXLINE];
}Packet;

int gsock;
int headerSize ;
struct sockaddr_in gserver;
Packet last;
Packet curr;
void die(char *);
void backup(Packet *,Packet *);
void printStatus(int ,Packet *);
void SENDTO(int,int);
void handle(int signo){
	if(signo == SIGALRM){
		//TIMEOUT
		alarm(TIMEOUT);
		SENDTO(gsock,1);
	}
}
int main(){
	//SET ALARM HANDLER
	struct sigaction act;
	act.sa_handler = handle;
	act.sa_flags = SA_RESTART; // restart the recvfrom call after interrupt handled
	sigaction(SIGALRM,&act,NULL);
	memset(curr.payload,'\0',MAXLINE);
	memset(last.payload,'\0',MAXLINE);
	headerSize =  sizeof(curr.size) +  sizeof(curr.seq_no)+ sizeof(curr.lst_pkt)+sizeof(curr.TYPE);
	
	//create UDP socket
	int sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock<0){die("Socket could not be created");}
	gsock  = sock;
	
	//Set server addr
	struct sockaddr_in server;
	server.sin_family =AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = inet_addr(ADDR);
	gserver = server;
	int len=sizeof(server);
	
	//Open file
	FILE * fp = fopen(FILENAME,"r");
	if(fp==NULL){die("Unable to read file");}
	
	int n;
	int seq = 0;
	int done = 0;
	while(!done && (n=fscanf(fp,"%[^\n]",curr.payload))!=EOF){
		fgetc(fp);//eat \n
		curr.seq_no = htonl(seq);
		int size = strlen(curr.payload)+headerSize;
		seq+=size;
		curr.size = htonl(size);
		if(feof(fp)){
			//EOF encountered
			//last packet
			curr.lst_pkt = '1';
			done = 1;
		}else{
				curr.lst_pkt ='0';
				done=0;
		}
		curr.TYPE = 'D';
		SENDTO(sock,0);
		//recv ACK
		alarm(TIMEOUT);//timer for recving
		Packet rcv;
		int r = recvfrom(sock,&rcv,sizeof(rcv),0,(struct sockaddr *)&server,&len);
		if(r<0){
				if(errno == EINTR){
					//alarm went off - auto restart at recvfrom
				}else{
						printf("%d\n",errno);
						die("Recv error");
				}
		}
		if(r==0){
			puts("Server terminated Connection");
			break;
		}
		alarm(0);//ACK received
		if(rcv.TYPE == 'A' && last.seq_no == rcv.seq_no){
				printStatus(2,&rcv);
		}else{
			die("SERVER CORRUPT RESPONSE");
		}
		usleep(100 * 1000); // 100ms delay
	}
	
	if(n<0){die("File READ ERROR");}
	fclose(fp);
	close(sock);
}
void die(char * c){
		perror(c);
		exit(1);
}
void backup(Packet *p1,Packet *p2){
		p2->size=p1->size;
		p2->seq_no = p1->seq_no;
		p2->lst_pkt = p1->lst_pkt;
		p2->TYPE = p1->TYPE;
		memset(p2->payload,'\0',MAXLINE);
		strcpy(p2->payload,p1->payload);
		memset(p1->payload,'\0',MAXLINE);
}
void printStatus(int type, Packet *p){
	if(type == 0){
		//SENT Packet
		printf("SENT Packet: Seq. No. = %d,Size = %d\n",ntohl(p->seq_no),ntohl(p->size));
		return;
	}
	if(type == 1){
		//RE_TRANSMIT Packet
		printf("RE_TRANSMIT Packet: Seq. No. = %d,Size = %d\n",ntohl(p->seq_no),ntohl(p->size));
		return;
	}
	if(type == 2){
		//ACK Packet
		printf("ACK Packet: Seq. No. = %d\n",ntohl(p->seq_no));
		return;
	}
}
void SENDTO(int sock,int type){
	int r;
	if(type == 0){
		backup(&curr,&last);
		//send data
		r = sendto(sock,&last,sizeof(last),0,(struct sockaddr *)&gserver,sizeof(gserver));
		if(r<0){die("SENDTO Error");}
		printStatus(type,&last);
	}else if(type == 1){
		//send data
		r = sendto(sock,&last,sizeof(last),0,(struct sockaddr *)&gserver,sizeof(gserver));
		if(r<0){die("SENDTO Error");}
		printStatus(type,&last);
	}
}