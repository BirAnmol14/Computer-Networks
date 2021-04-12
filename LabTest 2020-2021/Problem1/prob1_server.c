/*
Name: BIR ANMOL SINGH
ID: 2018A7PS0261P
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define ADDR "127.0.0.1"
#define PORT 11223
#define PDR 1 // 1-> 10 %, 2-> 20% and so on
#define MAXLINE 256 //asume that each line has a max size to be UDP safe

typedef struct pkt{
	int  size;
	int seq_no;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[MAXLINE];
}Packet;
void die(char *);
int randomDrop();
void printStatus(int,Packet *);
int main(){
	Packet ack,rcv;	
	int headerSize =  sizeof(ack.size) +  sizeof(ack.seq_no)+ sizeof(ack.lst_pkt)+sizeof(ack.TYPE);
	memset(ack.payload,'\0',MAXLINE);
	memset(rcv.payload,'\0',MAXLINE);
	
	//create UDP socket
	int sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock<0){die("Socket could not be created");}
	
	//Set server addr
	struct sockaddr_in serv;
	serv.sin_family =AF_INET;
	serv.sin_port = htons(PORT);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind the address
	int r = bind(sock,(struct sockaddr *)&serv,sizeof(serv));
	if(r<0){die("Bind error");}
		
	printf("Server Started on %s : %d\n",inet_ntoa(serv.sin_addr),ntohs(serv.sin_port));
	
	struct sockaddr_in cli; int len=sizeof(cli);
	int n;
	int done = 0;
	FILE * fp = fopen("output.txt","w");
	//creating output file
	if(fp==NULL){die("Failed to create output file");}
	
	//rcv packets from client closes till the end
	while(!done && (n = recvfrom(sock,&rcv,sizeof(rcv),0,(struct sockaddr *)&cli,&len))>0){
		//random drop packets
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
				fprintf(fp,"%s\n",rcv.payload);//output to file
				memset(rcv.payload,'\0',MAXLINE);
				usleep(100*1000);
				//SEND ACK
				ack.seq_no = rcv.seq_no;
				ack.size = htonl(headerSize);
				memset(ack.payload,'\0',MAXLINE);
				ack.lst_pkt = rcv.lst_pkt;
				ack.TYPE = 'A';
				
				r = sendto(sock,&ack,sizeof(ack),0,(struct sockaddr *)&cli,len);
				if(r<0){printf("%d\n",errno);die("SENDTO Error");}
				
				printStatus(1,&ack);
		}else{
			close(sock);
			fclose(fp);
			die("Corrupt Client");
		}
	}
	if(n<0){die("Recv error");}
	
	fclose(fp);
	printf("output.txt created successfully!\n");
	close(sock);
}

void die(char * c){
		perror(c);
		exit(1);
}
int randomDrop(){
	srand(time(0));
	int r = rand()%10+1;//1 to 10
	if(r<=PDR){
		return 1;
	}
	else {
		return 0;
	}
}
void printStatus(int type, Packet *p){
	if(type == 0){
			//RCVD Packet
			printf("RCVD Packet: Seq. No. = %d,Size = %d\n",ntohl(p->seq_no),ntohl(p->size));
			return;
	}
	if(type == 1){
			//SENT ACK Packet
			printf("SENT ACK: Seq. No. = %d\n",ntohl(p->seq_no));
			return;
	}
	if(type == 2){
			//DROP Packet
			printf("DROP Packet: Seq. No. = %d\n",ntohl(p->seq_no));
			return;
	}
}

