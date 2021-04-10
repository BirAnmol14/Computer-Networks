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
#define PORT 12121
#define P_SIZE 100

typedef struct pkt{
	int size;
	int seq_no;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[P_SIZE-2*sizeof(int)-2*sizeof(char)];
}PKT;

void die(char * c){
		perror(c);
		exit(1);
}
int randomDrop(int PDR){
	srand(time(0));
	int r = rand()%10+1;//1 to 10
	if(r<=PDR){
		return 1;
	}
	else {
		return 0;
	}
}
void printStatus(int type, PKT *p){
	if(type == 0){
			//RCVD PKT
			printf("RCVD PKT: Seq. No. = %d,Packet Size= %ld ,Payload Size = %d\n",ntohl(p->seq_no),sizeof(PKT),ntohl(p->size));
			return;
	}
	if(type == 1){
			//SENT ACK PKT
			printf("SENT ACK: Seq. No. = %d\n",ntohl(p->seq_no));
			return;
	}
	if(type == 2){
			//DROP PKT
			printf("DROP PKT: Seq. No. = %d\n",ntohl(p->seq_no));
			return;
	}
}


int main(){
	int PACKET_SIZE = P_SIZE;
	int PDR = 4;// 1-> 10 %, 2-> 20% and so on
	PKT ack,rcv;	
	int headerSize =  sizeof(ack.size) +  sizeof(ack.seq_no)+ sizeof(ack.lst_pkt)+sizeof(ack.TYPE);
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
		struct sockaddr_in cli; int len;
		int confd= accept(lsock,(struct sockaddr *)&cli,&len);
		if(confd<0){die("Accept Failed");}
		printf("Connected to Client %s : %d\n",inet_ntoa(cli.sin_addr),ntohs(cli.sin_port));
		char * buf;
		buf = malloc(1*sizeof(char));
		int last = 0;
		int size=1;
		int n;
		int done = 0;
		
		//rcv packets from client till the end
		while(!done && (n = recv(confd,&rcv,sizeof(rcv),0))>0){
			printStatus(0,&rcv);
			
			
			int drop = randomDrop(PDR);
			if(drop){
					//pkt drop 
					printStatus(2,&rcv);
					continue ;
			}
			
			if(rcv.TYPE == 'D'){
					//DATA pkt rcv
					if(rcv.lst_pkt == '1'){
						//last pkt
						done = 1;
					}else{
						done =0 ;
					}
					printStatus(0,&rcv);
					size = size + ntohl(rcv.size);
					buf = realloc(buf,size);
					strcpy(buf+last,rcv.payload);
					last = last + ntohl(rcv.size);
					memset(rcv.payload,'\0',payloadSize);
					
					//SEND ACK
					ack.seq_no = rcv.seq_no;
					ack.size = htonl(0);
					memset(ack.payload,'\0',payloadSize);
					ack.lst_pkt = rcv.lst_pkt;
					ack.TYPE = 'A';
					
					r = send(confd,&ack,sizeof(ack),0);
					if(r<0){die("SEND Error");}
					printStatus(1,&ack);
			}else{
				close(confd);
				close(lsock);
				die("Corrupt Client");
			}
		}
		if(n<0){die("Recv error");}
		//write buf to output file
		buf[size]='\0';
		char fname[256];
		sprintf(fname,"output%d.txt",file);
		FILE * fp = fopen(fname,"w");
		fprintf(fp,"%s",buf);
		fclose(fp);
		printf("%s created successfully!\n",fname);
		file++;
	}
	
	close(lsock);
}
