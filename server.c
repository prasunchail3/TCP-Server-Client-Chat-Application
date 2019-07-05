#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<stdlib.h>
#include<arpa/inet.h>

struct customData {
	struct sockaddr_in cli_addr;
};

int sockfd;
int limit, curr;
pthread_t writeTid, decideTid;
int* query;
int* newsockfd;
struct sockaddr_in serv_addr;
struct customData* look;

void* writeSocket(void* arg);
void readSocket();
void* decide(void* arg);
void establishSocket();
int arbitrator();
void error(const char*);

int main(int argc, char* argv[]) {
	
	if(argc != 3) 
		error("Command Line arguments not properly specified\n");
	
	int portno = atoi(argv[1]), i;
	limit = atoi(argv[2]);
	curr = 0;

	look = (struct customData*)malloc(limit * sizeof(struct customData));	
	newsockfd = (int*)malloc(limit * sizeof(int));
	query = (int*)malloc(limit * sizeof(int));
	for(i = 0; i < limit; i += 1)
		*(query + i) = 0;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) 
		error("Error opening socket\n");
	bzero((char*)&serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		error("Binding Failed\n");
	}

	listen(sockfd, limit);
	pthread_create(&writeTid, NULL, writeSocket, 0);
	//decide();
	pthread_create(&decideTid, NULL, decide, 0);
	pthread_join(writeTid, NULL);
	pthread_cancel(decideTid);
	
	return 0;
}

void* writeSocket(void* arg) {
	char buffer1[100];
	char buffer[100];
	int i, pos, n, j;
	char ch;
	while(1) {
		bzero(buffer1, 100);
		bzero(buffer, 100);
		buffer[0] = '0';
		i = 0;
		while((ch = getchar()) != EOF) {
			buffer1[i] = ch;
			i += 1;
		}
		buffer1[i] = 0;

		i = 0;
		pos = 0;
		while(buffer1[i] != ' ') {
			pos *= 10;
			pos += (buffer1[i] - '0');
			i += 1;
		}
		j = 1;
		while(buffer1[i] != '\0') {
			buffer[j] = buffer1[i];
			i += 1;
			j += 1;
		}
		if(pos == 0) {
			n = strcmp("0 q\n", buffer);
			if(n == 0) {
				for(i = 0; i < limit; i += 1) {
					if(*(query + i) == 0)
						continue;
					shutdown(*(newsockfd + i), 2);
					close(*(newsockfd + i));
					curr -= 1;
				}
				shutdown(sockfd, 2);
				close(sockfd);
				break;
			}
			for(i = 0; i < limit; i += 1) {
				if(*(query + i) == 0)
					continue;
				n = send(*(newsockfd + i), buffer, strlen(buffer), 0);
				if(n < 0)
					error("Error in writing");
			}
		}
		else {
			pos -= 1;
			if(pos >= limit || *(query + pos) == 0) {
				printf("User not connected to server\n");
				continue;
			}

			n = strcmp("0 q\n", buffer);
			if(n == 0) {
				buffer[0] = 'q';
				buffer[1] = 0;
			}

			n = write(*(newsockfd + pos), buffer, strlen(buffer));
			if(n < 0)
				error("Error in writing");
		}
	}
	exit(1);
}

int arbitrator() {
	int i;
	for(i = 0; i < limit; i += 1) {
		if(*(query + i) == 0) {
			*(query + i) = 1;
			curr += 1;
			return i;
		}
	}
}

void* decide(void* arg) {
	char buffer[100];
	char buffer1[100];
	fd_set readfds;
	int i, mV, activity;

	while(1) {
		FD_ZERO(&readfds);
		if(curr == limit)
			mV = 0;
		else {
			FD_SET(sockfd, &readfds);
			mV = sockfd;
		}

		for(i = 0; i < limit; i += 1) {
			if(*(query + i) == 0)
				continue;
			FD_SET(*(newsockfd + i), &readfds);
			if(*(newsockfd + i) > mV)
				mV = *(newsockfd + i);
		}

		activity = select(mV + 1, &readfds, NULL, NULL, NULL);

		if(FD_ISSET(sockfd, &readfds)) {
			//printf("ESTABLISH\n");
			establishSocket();
		}
		for(i = 0; i < limit; i += 1) {
			if(*(query + i) == 0)
				continue;
			if(FD_ISSET(*(newsockfd + i), &readfds)) { 
				//printf("READ\n");
				readSocket(i + 1);
			}
		}
	}
}

void establishSocket() {
	struct sockaddr_in cli_addr;
	socklen_t clilen = sizeof(cli_addr);
	int temp = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
	if(temp < 0)
		error("Error on Accept");
	int pos = arbitrator();
	*(newsockfd + pos) = temp;
	struct customData* p = (struct customData*)malloc(sizeof(struct customData));
	p -> cli_addr = cli_addr;
	*(look + pos) = *p;
	printf("%s : %d (%d) joined the chat\n", inet_ntoa(p -> cli_addr.sin_addr), p -> cli_addr.sin_port, (pos + 1));
}

void readSocket(int pos1) {
	char buffer[100];
	char buffer1[100];
	int n, i, pos, j;
	char ch;

	bzero(buffer, 100);
	bzero(buffer1, 100);

	n = recv(newsockfd[pos1 - 1], buffer1, 100, 0);

	if(n == 0) {
		printf("%s : %d (%d) left the chat\n", inet_ntoa((look + pos1 - 1) -> cli_addr.sin_addr), (look + pos1 - 1) -> cli_addr.sin_port, pos1);
		shutdown(*(newsockfd + pos1 - 1), 2);
		close(*(newsockfd + pos1 - 1));
		query[pos1 - 1] = 0;
		curr -= 1;
		return;
	}

	if(n < 0)
		error("Error in reading");

	j = 0;
	char temp[100];
	sprintf(temp, "%d", pos1);
	while(*(temp + j) != 0) {
		buffer[j] = temp[j];
		j += 1;
	}
	i = 0;
	pos = 0;
	while(buffer1[i] != ' ') {
		pos *= 10;
		pos += (buffer1[i] - '0');
		i += 1;
	}
	while(buffer1[i] != '\0') {
		buffer[j] = buffer1[i];
		i += 1;
		j += 1;
	}
	if(pos == 0) {
		printf("%s", buffer);
		//printf("%s : %d (%d) -> %s\n", inet_ntoa((look + pos1 - 1) -> cli_addr.sin_addr), (look + pos1 - 1) -> cli_addr.sin_port, pos1, buffer);	
	}
	else {
		pos -= 1;
		if(pos >= limit || *(query + pos) == 0) {
			printf("User not connected to server\n");
			return;
		}

		else {
			n = write(*(newsockfd + pos), buffer, strlen(buffer));
			if(n < 0)
				error("Error in writing");
		}
	}
}

void error(const char* msg) {
	perror(msg);
	exit(1);
}
