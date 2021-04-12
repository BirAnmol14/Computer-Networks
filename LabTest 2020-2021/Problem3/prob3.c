/*
Name: BIR ANMOL SINGH
ID: 2018A7PS0261P
CMD LINE ARGS: ./a.out <My ID> <MY PORT> <Next PEER PORT> 
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

#define PDR 1 // 1-> 10 %, 2-> 20% and so on
#define TIMEOUT 2
#define MAXLINE 256 //asume that each line has a max size to be UDP safe
#define ADDR "127.0.0.1"

typedef struct pkt{
	int size;
	int seq_no;
	int src;
	int dst;
	char lst_pkt;// '1' -> last ; '0' -> not last
	char TYPE;// 'D'->data, 'A'->ack
	char payload[MAXLINE];
}Packet;

int gsock;
int headerSize ;
int myId;
struct sockaddr_in gserver;
Packet last;
Packet curr;
void die(char *);
void backup(Packet *,Packet *);
void printStatus(int ,Packet *);
void SENDTO(int,int);
int randomDrop();

void handle(int signo){
	if(signo==SIGCHLD){
			wait(NULL);//no zombie
	}
	if(signo == SIGALRM){
		//TIMEOUT
		alarm(TIMEOUT);
		SENDTO(gsock,1);
	}
}
int main(int argc, char * argv[]){
	if(argc<4||argc>4){
		die("Inavalid argument use: ./a.out <My ID> <MY PORT> <Next PEER PORT> ");
	}
	myId=atoi(argv[1]);
	int port = atoi(argv[2]);
	int peer = atoi(argv[3]);
	//SET ALARM HANDLER
	struct sigaction act;
	act.sa_handler = handle;
	act.sa_flags = SA_RESTART; // restart the recvfrom call after interrupt handled
	sigaction(SIGALRM,&act,NULL);
	memset(curr.payload,'\0',MAXLINE);
	memset(last.payload,'\0',MAXLINE);
	headerSize =  sizeof(curr.size) +  sizeof(curr.seq_no)+ sizeof(curr.lst_pkt)+sizeof(curr.TYPE)+sizeof(curr.src)+sizeof(curr.dst);
	//create UDP socket
	int sock = socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(sock<0){die("Socket could not be created");}
	gsock  = sock;
	//Bind yourself to a port
	struct sockaddr_in me;
	me.sin_family= AF_INET;
	me.sin_port=htons(port);
	me.sin_addr.s_addr = htonl(INADDR_ANY);
	int r  = bind(sock,(struct sockaddr *)&me,sizeof(me));
	if(r<0){die("Bind");}
	printf("Client Started on %s : %d\n",inet_ntoa(me.sin_addr),ntohs(me.sin_port));
	struct sockaddr_in other;
	other.sin_family= AF_INET;
	other.sin_port=htons(peer);
	other.sin_addr.s_addr = inet_addr(ADDR);
	gserver = other;
	int len = sizeof(me);
	int pid = fork();
	if(pid==0){
		//DOWNLOAD FILE
		int first = 1;
		Packet rcv,ack;
		memset(rcv.payload,'\0',MAXLINE);
		memset(ack.payload,'\0',MAXLINE);
		struct sockaddr_in cli; int len=sizeof(cli);
		int n;
		int done = 0;
		FILE * fp;
		//rcv packets from client closes till the end
		while(!done && (n = recvfrom(sock,&rcv,sizeof(rcv),0,(struct sockaddr *)&cli,&len))>0){
			if(first && myId == ntohl(rcv.dst)){
				first=0;
				fp = fopen("output.txt","w");
				//creating output file
				if(fp==NULL){die("Failed to create output file");}
			}
			//random drop packets
			int drop = randomDrop(PDR);
			if(drop){
					//pkt drop 
					printStatus(5,&rcv);
					continue ;
			}
			
		
			if(rcv.TYPE == 'D' && myId == ntohl(rcv.dst)){
				//DATA pkt rcv
				if(rcv.lst_pkt == '1'){
					//last pkt
					done = 1;
				}else{
					done =0 ;
				}
				printStatus(4,&rcv);
				fprintf(fp,"%s\n",rcv.payload);//output to file
				memset(rcv.payload,'\0',MAXLINE);
				usleep(100*1000);
				//SEND ACK
				ack.seq_no = rcv.seq_no;
				ack.size = htonl(headerSize);
				ack.src = htonl(myId);
				ack.dst = rcv.src;
				memset(ack.payload,'\0',MAXLINE);
				ack.lst_pkt = rcv.lst_pkt;
				ack.TYPE = 'A';
				r = sendto(sock,&ack,sizeof(ack),0,(struct sockaddr *)&other,len);
				if(r<0){printf("%d\n",errno);die("SENDTO Error");}
				printStatus(6,&ack);
			}else{
				printf("Relay Link: %d\n",myId);
				r = sendto(sock,&rcv,sizeof(rcv),0,(struct sockaddr *)&other,len);
				if(r<0){printf("%d\n",errno);die("SENDTO Error");}
				printStatus(7,&rcv);
			}
			
		}
		if(n<0){die("Recv error");}
		fclose(fp);
		printf("output.txt created successfully!\n");
		close(sock);
		kill(getppid(),SIGKILL);
		exit(1);
	}else{
		//UPLOAD FILE
		printf("Enter filename: ");
		char fname[256];
		scanf("%s",fname);getchar();
		printf("Enter dest id: ");
		int d;
		scanf("%d",&d);
		getchar();
		FILE * fp = fopen(fname,"r");
		if(fp==NULL){die("Unable to read file");}
		kill(pid,SIGKILL);//cause you have to upload
		int n;
		int seq = 0;
		int done = 0;
		struct sockaddr_in cli;
		curr.src = htonl(myId);
		curr.dst = htonl(d);
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
			int r = recvfrom(sock,&rcv,sizeof(rcv),0,(struct sockaddr *)&cli,&len);
			if(r<0){
					if(errno == EINTR){
						//alarm went off - auto restart at recvfrom
					}else{
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
		exit(1);
	}
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
		p2->src = p1->src;
		p2->dst = p1->dst;
		memset(p2->payload,'\0',MAXLINE);
		strcpy(p2->payload,p1->payload);
		memset(p1->payload,'\0',MAXLINE);
}
void printStatus(int type, Packet *p){
	if(type == 0){
		//SENT Packet
		printf("SENT Packet: Node ID: %d,Seq. No. = %d,Size = %d,Src=%d, Dst=%d\n",myId,ntohl(p->seq_no),ntohl(p->size),ntohl(p->src),ntohl(p->dst));
		return;
	}
	if(type == 1){
		//RE_TRANSMIT Packet
		printf("RE_TRANSMIT Packet: Node ID: %d, Seq. No. = %d,Size = %d\n",myId,ntohl(p->seq_no),ntohl(p->size));
		return;
	}
	if(type == 2){
		//ACK Packet RECV
		printf("ACK Packet: Node ID: %d, Seq. No. = %d\n",myId,ntohl(p->seq_no));
		return;
	}
	if(type == 4){
		//RCVD Packet
		printf("RCVD Packet: Node ID: %d, Seq. No. = %d,Size = %d\n",myId,ntohl(p->seq_no),ntohl(p->size));
		return;
	}
	if(type == 5){
		//DROP Packet
		printf("DROP Packet: Node ID:%d, Seq. No. = %d\n",myId,ntohl(p->seq_no));
		return;
	}
	if(type == 6){
		//SENT ACK Packet
		printf("SENT ACK: Node ID:%d, Seq. No. = %d,Src=%d, Dst=%d\n",myId,ntohl(p->seq_no),ntohl(p->src),ntohl(p->dst));
		return;
	}
	if(type == 7){
			//Relay Packet
			printf("Relay: Node ID:%d, Seq. No. = %d, Type=%s\n",myId,ntohl(p->seq_no),p->TYPE=='D'?"DATA":"ACK");
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