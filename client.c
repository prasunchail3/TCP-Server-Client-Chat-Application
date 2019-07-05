#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<stdlib.h>
#include<netdb.h>

int sockfd;
struct sockaddr_in serv_addr;
struct hostent* server;
pthread_t writeTid;
pthread_t readTid;

void error(const char*);
void* readSocket(void* arg);
void* writeSocket(void* arg);

int main(int argc, char* argv[]) {
	if(argc != 3) {
		error("Command Line arguments not properly specified\n");
	}
	int i;
	int portno = atoi(argv[2]);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) 
		error("Error opening socket\n");

	server = gethostbyname(argv[1]);
	if(server == NULL)
		error("Error, no such host\n");

	bzero((char*)&serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server-> h_addr,(char *)&serv_addr.sin_addr.s_addr , server-> h_length);
	serv_addr.sin_port = htons(portno);

	if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
		error("Connection Failed");

	pthread_create(&writeTid, NULL, writeSocket, 0);
	pthread_create(&readTid, NULL, readSocket, 0);
	pthread_join(writeTid, NULL);
	pthread_join(readTid, NULL);

	return 0;
}

void error(const char* msg) {
	perror(msg);
	exit(1);
}

void* readSocket(void* arg) {
	char buffer[100];
	int n;
	while(1) {
		bzero(buffer, 100);
		n = recv(sockfd, buffer, 100, 0);
		if(n == 0) {
			bzero(buffer, 100);
			buffer[0] = 0;
			break;
		}
		if(n < 0)
			error("Error in reading");
		if(strcmp("q", buffer) == 0)
			break;
		printf("%s", buffer);
	}
	shutdown(sockfd, 2);
	close(sockfd);
	exit(1);	
}
void* writeSocket(void* arg) {
	char buffer[100];
	int n, i;
	char ch;
	while(1) {
		bzero(buffer, 100);
		i = 0;
		while((ch = getchar()) != EOF) {
			buffer[i] = ch;
			i += 1;
		}
		buffer[i] = 0;

		n = strcmp("q\n", buffer);
		if(n == 0) {
			bzero(buffer, 100);
			buffer[0] = 0;
			break;
		}

		n = send(sockfd, buffer, strlen(buffer), 0);
		if(n < 0)
			error("Error in writing");
	}
	shutdown(sockfd, 2);
	close(sockfd);
	exit(1);
}
