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
		int done = 0;
		
		fd_set rset;
		FD_ZERO(&rset);
		FD_SET(confd0,&rset);
		FD_SET(confd1,&rset);
		
		int max = confd0>confd1?confd0:confd1;
		int last_seq =-1 ;
		int last_ack=0;//last in order ACK
		BUF buffer;
		buffer.buf;
		buffer.count = 1;
		buffer.last = -1;
		int buffered=0;
		int isPending1=1;
		int isPending0 = 1;
		while(1){
			printf("Last Inorder:%d last seq:%d\n",last_ack,last_seq);
			if(done && !buffered && (last_ack == last_seq)){
				puts("Here");
				break;
			}	
			if(done && buffered){
				if(last_ack + PACKET_SIZE == ntohl(buffer.buf[0].seq_no)){
					for(int i=0;i<buffered;i++){
						last_ack = ntohl(buffer.buf[i].seq_no);
						fprintf(fp,"%s",buffer.buf[i].payload);
						memset(buffer.buf[i].payload,'\0',payloadSize);
					}
				}
			}
			int max = -1;
			if(isPending0){
				FD_SET(confd0,&rset);
				max = max>confd0?max:confd0;
			}
			if(isPending1){
				FD_SET(confd1,&rset);
				max = max>confd0?max:confd0;
			}
			
			//rcv packets from client till the end
			select(max+1,&rset,NULL,NULL,NULL);
			
			if(FD_ISSET(confd0,&rset)){
				PKT rcv;
				PKT ack;
				int confd = confd0;
				if((n = recv(confd0,&rcv,sizeof(rcv),0))>0){
					printStatus(0,&rcv);
					delay(100);
					if(rcv.TYPE == 'D'){
							//DATA pkt rcv
							if(rcv.lst_pkt == '1'){
								//last pkt
								done = 1;
								last_seq = ntohl(rcv.seq_no);
								FD_CLR(confd0,&rset);
							}else{
								done =0 ;
								FD_SET(confd0,&rset);
							}
							
							if(last_ack+PACKET_SIZE == ntohl(rcv.seq_no)){
									//In order
									last_ack = ntohl(rcv.seq_no);
									fprintf(fp,"%s",rcv.payload);
									if(buffered && last_ack + PACKET_SIZE == ntohl(buffer.buf[0].seq_no)){
										for(int i=0;i<buffered;i++){
											last_ack = ntohl(buffer.buf[i].seq_no);
											fprintf(fp,"%s",buffer.buf[i].payload);
											memset(buffer.buf[i].payload,'\0',payloadSize);
										}
										buffered = 0;
										buffer.last = -1;
									}
							}else{
									//Buffer the Packet
									buffered++;
									buffer.last++;
									if(buffer.last > buffer.count){
											buffer.count = buffer.count+1;
					
									}
									buffer.buf[buffer.last] = rcv;
									strcpy(buffer.buf[buffer.last].payload,rcv.payload);
							}
							memset(rcv.payload,'\0',payloadSize);
							
							//SEND ACK
							ack.seq_no = rcv.seq_no;
							ack.size = htonl(0);
							ack.channel = htonl(0);
							memset(ack.payload,'\0',payloadSize);
							ack.lst_pkt = rcv.lst_pkt;
							ack.TYPE = 'A';
							r = send(confd0,&ack,sizeof(ack),0);
							if(r<0){die("SEND Error");}
							printStatus(1,&ack);
					}else{
						close(confd0);
						close(lsock);
						die("Corrupt Client");
					}
				}
				else if(n==0){
					close(confd0);
				}
			}if(FD_ISSET(confd1,&rset)){
				
				PKT rcv;
				PKT ack;
				int confd = confd1;
				if((n = recv(confd1,&rcv,sizeof(rcv),0))>0){
					delay(100);
					printStatus(0,&rcv);
					if(rcv.TYPE == 'D'){
							//DATA pkt rcv
							if(rcv.lst_pkt == '1'){
								//last pkt
								done = 1;
								last_seq = ntohl(rcv.seq_no);
								FD_CLR(confd1,&rset);
							}else{
								done =0 ;
								FD_SET(confd1,&rset);
							}

							if(last_ack+PACKET_SIZE == ntohl(rcv.seq_no)){
									//In order
									last_ack = ntohl(rcv.seq_no);
									fprintf(fp,"%s",rcv.payload);
									if(buffered && last_ack + PACKET_SIZE == ntohl(buffer.buf[0].seq_no)){
										for(int i=0;i<buffered;i++){
											last_ack = ntohl(buffer.buf[i].seq_no);
											fprintf(fp,"%s",buffer.buf[i].payload);
											memset(buffer.buf[i].payload,'\0',payloadSize);
										}
										buffered = 0;
										buffer.last = -1;
									}
							}else{
									//Buffer the Packet
									buffered++;
									buffer.last++;
									if(buffer.last > buffer.count){
											buffer.count = buffer.count+1;
									}
									buffer.buf[buffer.last] = rcv;
									strcpy(buffer.buf[buffer.last].payload,rcv.payload);
							}
							memset(rcv.payload,'\0',payloadSize);
							
							//SEND ACK
							ack.seq_no = rcv.seq_no;
							ack.size = htonl(0);
							ack.channel = htonl(1);
							memset(ack.payload,'\0',payloadSize);
							ack.lst_pkt = rcv.lst_pkt;
							ack.TYPE = 'A';
							r = send(confd1,&ack,sizeof(ack),0);
							if(r<0){die("SEND Error");}
							printStatus(1,&ack);
					}else{
						close(confd1);
						close(lsock);
						die("Corrupt Client");
					}
				}else if(n==0){
					close(confd1);
				}
			}
		}		
		fclose(fp);
		printf("%s created successfully!\n",fname);
		close(confd1);
		close(confd0);
		file++;
	}
	
	close(lsock);
}
