#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>

#define USER_LIMIT 100
#define CHANNEL_COUNT 10000
#define FRAME_SIZE 24
#define BUFFER_SIZE 8192
#define PACKAGE_SIZE 32768

struct user {
	int active;
	uuid_t uuid;
	char ip[INET_ADDRSTRLEN];
	int port;
	time_t lastup;
	struct sockaddr_in *dgram;
} typedef User;

struct package {
	int op, ch;
	uuid_t uuid;
	ssize_t size;
	char *data;
} typedef Package;

sem_t quesem;
User user[CHANNEL_COUNT][USER_LIMIT];
Package package[BUFFER_SIZE];

char* uuidstring(uuid_t uuid) //TODO: DELET DIS
{
	static char *str = NULL;
	if(!str)
		str = malloc(37);
	uuid_unparse(uuid, str);
	return str;
}

void* receive(void *param)
{
	struct sockaddr_in saddr, caddr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	memset(&saddr, 0, addrlen);
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = INADDR_ANY;
	
	saddr.sin_port = htons(1873);
	int sfd = socket(AF_INET, SOCK_DGRAM, 0);
	fcntl(sfd, F_SETFD, FD_CLOEXEC);
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	bind(sfd, (struct sockaddr*)&saddr, addrlen);
	
	int index = 0;
	char data[PACKAGE_SIZE];
	
	while(1)
	{
		printf("Waiting for packet...\n");
		
		package[index].size =
		  recvfrom(sfd, data, PACKAGE_SIZE, 0, (struct sockaddr*)&caddr, &addrlen);
		package[index].op = ntohl(*(int*)data);
		package[index].ch = ntohl(*(int*)(data + sizeof(int)));
		memcpy(&package[index].uuid, data + sizeof(int) * 2, sizeof(uuid_t));
		
		package[index].data = malloc(package[index].size - FRAME_SIZE);
		memcpy(&package[index].data, data + FRAME_SIZE, package[index].size - FRAME_SIZE);
		
		index = (index + 1) % BUFFER_SIZE;
		sem_post(&quesem);
		
		printf("Received packet!\n");
	}
}

void* broadcast(void *param)
{
	int index = 0;
	
	while(1)
	{
		sem_wait(&quesem);
		printf("lolololol\n");
		free(package[index].data);
		index = (index + 1) % BUFFER_SIZE;
	}
}

int main()
{
	sem_init(&quesem, 0, 0);
	pthread_t receiver, broadcaster;
	pthread_create(&receiver, NULL, receive, NULL);
	pthread_create(&broadcaster, NULL, broadcast, NULL);
	
	while(1)
	{
		char cmd;
		scanf(" %c", &cmd);
		
		if(cmd == 'e' || cmd == 'q')
			break;
	}
}
