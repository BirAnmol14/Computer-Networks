//BASIC HEADERS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
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

#define ADDR "127.0.0.1"
#define PORT 10345
#define P_SIZE 100

typedef struct pkt{
	int size;
	int seq_no;
	int channel;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[P_SIZE-3*sizeof(int)-2*sizeof(char)];
}PKT;
typedef struct BUF{
	PKT buf[1000];
	int count;
	int last;
}BUF;
void delay(int ms)
{
    clock_t start_time = clock(); 
    while (clock() < start_time + ms);
	usleep(100*1000);
}
  
void die(char * c){
		perror(c);
		exit(1);
}

void printStatus(int type, PKT *p){
	if(type == 0){
			//RCVD PKT
			printf("RCVD PKT: Seq. No. = %d,Packet Size= %ld ,Payload Size = %d, CH = %d\n",ntohl(p->seq_no),sizeof(PKT),ntohl(p->size),ntohl(p->channel));
			return;
	}
	if(type == 1){
			//SENT ACK PKT
			printf("SENT ACK: Seq. No. = %d, CH = %d\n",ntohl(p->seq_no),ntohl(p->channel));
			return;
	}
	if(type == 2){
			//DROP PKT
			printf("DROP PKT: Seq. No. = %d, CH = %d\n",ntohl(p->seq_no),ntohl(p->channel));
			return;
	}
}


int main(){
	int PACKET_SIZE = P_SIZE;
	
	PKT ack,rcv;	
	int headerSize =  sizeof(ack.size) +  sizeof(ack.seq_no)+sizeof(ack.channel)+ sizeof(ack.lst_pkt)+sizeof(ack.TYPE);
	int payloadSize = PACKET_SIZE -headerSize ;
	int LISTQ=5;
	if(payloadSize<=0){
		die("Invalid Packet Size");
	}
	memset(ack.payload,'\0',payloadSize);
	memset(rcv.payload,'\0',payloadSize);
	//create TCP socket
	int lsock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(lsock<0){die("Socket could not be created");}
	
	//Set server addr
	struct sockaddr_in serv;
	serv.sin_family =AF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind the address
	int r = bind(lsock,(struct sockaddr *)&serv,sizeof(serv));
	if(r<0){die("Bind error");}
	
	//Listen for connections on server
	r = listen(lsock,LISTQ);
	if(r<0){die("Listen error");}
	
	printf("Server Up on %s : %d\n",inet_ntoa(serv.sin_addr),ntohs(serv.sin_port));
	int file =1;
	while(1){
		struct sockaddr_in cli0,cli1; int len;
		int confd0= accept(lsock,(struct sockaddr *)&cli0,&len);
		if(confd0<0){die("Accept Failed");}
		printf("Connected to Client(CH0)\n");
		int confd1= accept(lsock,(struct sockaddr *)&cli1,&len);
		if(confd1<0){die("Accept Failed");}
		printf("Connected to Client(CH1)\n");
		char fname[256];
		sprintf(fname,"output%d.txt",file);
		FILE * fp = fopen(fname,"w");
	
		int n;
		
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(confd0,&rset);
		FD_SET(confd1,&rset);
		
		int max = confd0>confd1?confd0:confd1;
		int pending[2];
		pending[0] = pending[1] = 1;
		int sock[2];
		sock[0] = confd0;
		sock[1] = confd1;
		int done = 0;
		int last_seq=-1;
		while(1){
			PKT rcv[2];
			PKT snd[2];
			if(done){
				break;
			}
			max = -1;
			for(int i=0;i<2;i++){
				memset(rcv[i].payload,'\0',payloadSize);
				memset(snd[i].payload,'\0',payloadSize);
				snd[i].seq_no=-1;
				if(pending[i]){
				//Await data
				FD_SET(sock[i],&rset);
					if(max<sock[i])max=sock[i];
				}
			}
			select(max+1,&rset,NULL,NULL,NULL);
			for(int i=0;i<2;i++){	
				if(FD_ISSET(sock[i],&rset)){
					if((n=recv(sock[i],&rcv[i],sizeof(PKT),0))>0){
						printStatus(0,&rcv[i]);
						snd[i].size = htonl(0);
						snd[i].seq_no = rcv[i].seq_no;
						snd[i].channel = rcv[i].channel;
						snd[i].TYPE = 'A';
						snd[i].lst_pkt = rcv[i].lst_pkt;
						if(rcv[i].lst_pkt=='1'){
							//last data received
							last_seq = ntohl(rcv[i].seq_no);
							FD_CLR(sock[i],&rset);
							pending[i] = 0;
						}
						fprintf(fp,"%s",rcv[i].payload);
						delay(100);
					}else if(n==0){
						//close(sock[i]);
					}else{
						die("Recv Error");
					}						
				}
			}
			for(int i=0;i<2;i++){
				//send ACK
				if(snd[i].seq_no==-1){
					continue;
				}
				send(sock[i],&snd[i],sizeof(PKT),0);
				printStatus(1,&snd[i]);
				if(ntohl(snd[i].seq_no)==last_seq){
					//last ack sent
					done = 1;
				}
			}
		}
		fclose(fp);
		printf("%s created successfully!\n",fname);
		file++;
	}
	
	close(lsock);
}
