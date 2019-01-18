//gcc server.c -o server.o -lm -lpq -lpthread -Wall -Ofast

//create table users(id serial primary key, mail text unique not null, pass text not null, sess bigint unique, online boolean, seen timestamp with time zone, name text, surname text, gender text, birthday date, birthplace text, residence text, education text, school text, department text, orientation text, photo bytea, interests text[]);
//create table relations(id serial primary key, user1 int not null, user2 int not null, beginning timestamp with time zone, duration int, confirm1 boolean, confirm2 boolean, unlocked boolean);
//create table messages(id bigserial primary key, sender int not null, receiver int not null, message text not null, sent timestamp with time zone, received timestamp with time zone, read timestamp with time zone);

#include "list.c"

#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <postgresql/libpq-fe.h>

//#include <netinet/in.h>
//#include <openssl/bio.h>
//#include <openssl/err.h>
//#include <openssl/applink.c>

unsigned int sfd,port=5434;
char *timestamp;
struct sockaddr_in saddr;
PGconn *conn;

//Reminder: Memory Leak
void* timeloop(void *param)
{
	int len=10;
	time_t now;
	struct tm ts;
	timestamp=malloc(len);

	while(1)
	{
		time(&now);
		ts = *localtime(&now);
		//"%a %Y-%m-%d %H:%M:%S %Z"
		strftime(timestamp,len,"%H:%M:%S",&ts);
		sleep(1);
	}
}

char* getclientip(int cfd)
{
	struct sockaddr_in caddr;
	socklen_t addrlen=sizeof(caddr);
	getpeername(cfd,(struct sockaddr *)&caddr,&addrlen);
	return inet_ntoa(caddr.sin_addr);
}

void heartbeat(int cfd, char *buffer)
{
	//printf("%s %s:\tHeartbeat signal.\n", getclientip(cfd), gethumantime());
	int res=send(cfd,"\0",1,0);
	if(res==-1) return;
}

void session(int cfd, char *buffer)
{
	//printf("%s %s:\tSession check.\n", gethumantime(), getclientip(cfd));
	int res=send(cfd,"a",1,0);
	if(res==-1) return;
}

void* communicate(void* param)
{
	unsigned int len,stat,cfd=*((int*)param);

	while(1)
	{
		if((stat=recv(cfd,&len,1,MSG_WAITALL))<=0) break;
		char cmd,*buffer=malloc(len);
		if((stat=recv(cfd,buffer,len,MSG_WAITALL))<=0) break;
		cmd=buffer[0];
		
		if(cmd==0)
			heartbeat(cfd,buffer);
		else if(cmd==1)
			session(cfd,buffer);
		//else if(cmd==1)
			//signup(cfd,buffer);
		//else if(cmd==2)
			//signin(cfd,buffer);
		
		free(buffer);
	}
	
	//printf("%s %s:\tDisconnected!\n", gethumantime(), getclientip(cfd));
	return NULL;
}

void* operate(void* param)
{
	while(1)
	{
		struct sockaddr caddr;
		socklen_t addrlen=sizeof(caddr);
		int cfd=accept(sfd,(struct sockaddr*)&caddr,&addrlen);
		//printf("%s %s:\tConnected!\n", gethumantime(), getclientip(cfd));

		pthread_t thread;
		pthread_create(&thread,NULL,communicate,(void*)&cfd);
	}
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	conn = PQconnectdb("");

	pthread_t timethread, serverthread;

	saddr.sin_family=AF_INET;
	saddr.sin_addr.s_addr=INADDR_ANY;
	saddr.sin_port=htons(port);

	sfd=socket(AF_INET,SOCK_STREAM,0);
	setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,(int[]){1},4);
	bind(sfd,(struct sockaddr*)&saddr,sizeof(saddr));
	listen(sfd,SOMAXCONN);

	//printf("%s 127.0.0.1:\t\tListening...\n", gethumantime());
	pthread_create(&timethread,NULL,timeloop,NULL);
	pthread_create(&serverthread,NULL,operate,NULL);
	sleep(2);

	while(1)
	{
		int x;
		scanf("%d", &x);
		if(x==0) break;
	}

	PQfinish(conn);
	close(sfd);
}

void* dataconn(void *param)
{
	printlg(LOG_NOTICE, "Started UDP thread.");
	
	struct sockaddr_in saddr, caddr, daddr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	memset(&saddr, 0, addrlen);
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(1873);
	
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	bind(sfd, (struct sockaddr*)&saddr, addrlen);
	
	PNode node;
	int port, ch, datalen, buffsize = 65535;
	char *ip = malloc(INET_ADDRSTRLEN), *data = malloc(buffsize + sizeof(int));
	
	printlg(LOG_DEBUG, "Initialized UDP server.");
	
	while(1) //Incomplete!!!
	{
		recvfrom(sfd, &ch, 4, 0, (struct sockaddr*)&caddr, &addrlen);
		ch = ntohl(ch);
		node = channel[ch]->next;
		datalen = recvfrom(sfd, data, buffsize, 0, (struct sockaddr*)&caddr, &addrlen);
		inet_ntop(AF_INET, &caddr.sin_addr, ip, INET_ADDRSTRLEN);
		port = ntohs(caddr.sin_port);
		
		while(node)
		{
			if(strcmp(ip, node->ip))
			{
				//Initialize destination address here!
				sendto(sfd, data, datalen, 0, (struct sockaddr*)&daddr, addrlen);
			}
			else
			{
				node->port = port;
				node->last = time(NULL);
			}
		}
	}
}

void cleanup()
{
	pthread_cancel(datafunc);
	pthread_cancel(metafunc);
	
	for(int i=0; i<chcount; i++)
	{
		PNode node = channel[i];
		while(node->next)
		{
			node = node->next;
			pthread_cancel(node->thread);
		}
	}
	
	for(int i=0; i<chcount; i++)
	{
		PNode node = channel[i];
		while(node->next)
		{
			node = node->next;
			pthread_join(node->thread, NULL);
		}
	}
	
	pthread_join(datafunc, NULL);
	pthread_join(metafunc, NULL);
}

		client->port = ntohs(caddr.sin_port);
		client->ip = malloc(INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &caddr.sin_addr, client->ip, INET_ADDRSTRLEN);
