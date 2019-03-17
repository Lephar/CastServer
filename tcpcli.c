#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

int main(int argc, char **argv)
{
	int sockfd, portno;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	unsigned char len, *data = malloc(255);
	
	portno = 641;
	hostname = "127.0.0.1";
	//hostname = "127.0.0.1";
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server = gethostbyname(hostname);
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(portno);
	connect(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
	
	while(1)
	{
		printf("Please enter length: ");
		scanf(" %d", &len);
		write(sockfd, &len, 1);
		printf("Please enter message: ");
		scanf(" %s", data);
		write(sockfd, data, len);
	}
	
	close(sockfd);
	return 0;
}

	/*n = write(sockfd, buf, strlen(buf));
	bzero(buf, BUFSIZE);
	n = read(sockfd, buf, BUFSIZE);
	printf("Echo from server: %s", buf);*/