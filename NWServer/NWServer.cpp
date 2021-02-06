/*  File Name : NWServer.cpp
    Author : Arup Sarker
    Creation date : 12/28/2020
    Description: Main responsibilities of this source are
    1. Create socket and bind with client to a specific port
    2. Receive file from client concurrently
    3. Utility function to check file integrity by calculating checkSum

    Notes: BSD socket apis are used and compiled with g++ in Ubuntu 18.04. For Other OS, files needs be cross compiled and create executable
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>

#define MAX_LENGTH_OF_SINGLE_LINE 4096

#define LISTEN_PORT 8050

#define SERVER_PORT 5080

#define BUFFER_SIZE 100000
#define MAX_NAME 1000
#define MAX_THREAD 100
// ToDo: This will be Dynamic; 
// Status: Done

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void* socketThreadFT(void *arg);


//CheckSum Calculation
unsigned calculateChecksum(void *buffer, size_t len)
{
	unsigned char *buf = (unsigned char *)buffer;
	size_t i;
	unsigned int checkSum = 0;

	for (i = 0; i < len; ++i)
	    checkSum += (unsigned int)(*buf++);
	return checkSum;
}

// This is a utility function for writing data into file

unsigned int fileWriter(char *fileName, int sockfd, FILE *fp) {
	// 11.1 Declare data length
	int n;
	// 11.2 Declare and Initialize buffer size

	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char s[64];
	assert(strftime(s, sizeof(s), "%c", tm));
	//printf("%s\n", s);

	printf("%s file Receiving is started in Connection ID: %d at :  %s\n", fileName, sockfd, s);

	// 11.3 Receive data from socket and save into buffer
	unsigned int checkSum = 0;
	char buff[BUFFER_SIZE] = {0};
	while(1) {		
		n = recv(sockfd, buff, BUFFER_SIZE,0);
		//printf("File is receiving from Socket : %d\n", sockfd);
		if (n == -1) {
			perror("Error in Receiving File");
			exit(1);
		}
		if(n <= 0) {
			break;
		}

		checkSum += calculateChecksum(buff, n);

		// 11.4 Read File Data from buffer and Write into file
		if (fwrite(buff, sizeof(char), n, fp) != n) {
			perror("Error in Writing File");
			exit(1);
		}

		// 11.5 Allocate buffer
		memset(buff, '\0', BUFFER_SIZE);
	}
	fclose(fp);
	return checkSum;
}



void* socketThreadFT(void *arg)
{
	int connfd = *((int *)arg);
	unsigned int checkSum = 0;

	pthread_mutex_lock(&lock);
	 // 8. Receive incoming file from client
	char filename[MAX_NAME] = {0};

	if (recv(connfd, filename, MAX_NAME,0) == -1) {
		perror("Unable to receive file due to connection or file error");
		exit(1);
	}

	int fileLength = strlen(filename);
	pthread_mutex_unlock(&lock);
	sleep(1);

	if(fileLength > 0) {
		// 9. After receiving from client, create file to save data

		//printf("File Name : %s\n", filename);
		FILE *fp = fopen(filename, "wb");
		if (fp == NULL) {
			perror("Unable to create File pointer");
			exit(1);
		}

		// 10. Start receiving file Data and print in console
		char addr[INET_ADDRSTRLEN];

		// 11. Write the data into file.

		checkSum = fileWriter(filename, connfd, fp);

		// 12. Print success message into console
		
		time_t t = time(NULL);
		struct tm *tm = localtime(&t);
		char s[64];
		assert(strftime(s, sizeof(s), "%c", tm));

		printf("%s is received with checkSum %d at :  %s\n",filename, checkSum,s);

	}

	close(connfd);	       
	pthread_exit(NULL);

}



int main(int argc, char *argv[]) {
	// 1. Create TCP Socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Unable to create Socket");
		exit(1);
	}
	if (pthread_mutex_init(&lock, NULL) != 0) { 
        	printf("mutex init has failed\n"); 
        	exit(1); 
    	} 

	// 2. Setup information about client and server
	struct sockaddr_in nwClientAddr, nwServerAddr;
	memset(&nwServerAddr, 0, sizeof(nwServerAddr));

	// 3. IP address should be IPv4
	nwServerAddr.sin_family = AF_INET;
	nwServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nwServerAddr.sin_port = htons(SERVER_PORT);

	// 4. Bind to Server Address
	if (bind(sockfd, (const struct sockaddr *) &nwServerAddr, sizeof(nwServerAddr)) == -1) {
		perror("Unable to Bind");
		exit(1);
	}

	// 5. Listen Socket for incoming File
	if (listen(sockfd, LISTEN_PORT) == -1) {
		perror("Error in Socket Listening");
		exit(1);
	}
	

	// 6. Accept connection for concurrency number
	socklen_t adlen = sizeof(nwClientAddr);
	int cfd = accept(sockfd, (struct sockaddr *) &nwClientAddr, &adlen);
	if (cfd == -1) {
		perror("Error in Accepting Connection for ");
		exit(1);
	}
	int val = 0;
	pthread_mutex_lock(&lock);

	char conString[MAX_NAME] = {0};
	if (recv(cfd, conString, MAX_NAME,0) == -1) {
		perror("Unable to receive concurrency due to connection or file error");
		exit(1);
	}
	char *numstr = basename(conString);
	val = atoi(numstr);
	printf("Number of files to receive: %d\n", val);
	close(cfd);
	pthread_mutex_unlock(&lock);
	

	
	int *new_sock;
	pthread_t tid[60];
	int idx = 0;
	while(1) {

		// 7. Accept connection for creating multi-thread
		socklen_t addrlen = sizeof(nwClientAddr);
		int connfd = accept(sockfd, (struct sockaddr *) &nwClientAddr, &addrlen);
		if (connfd == -1) {
			perror("Error in Accepting Connection");
			exit(1);
		}

		new_sock = (int*)malloc(1);
		*new_sock = connfd;
		//printf("Connection ID: %d\n", *new_sock);
		// 8. Multi-Thread creation
		if(pthread_create(&tid[idx], NULL, socketThreadFT, (void*)new_sock) < 0 ) {
			printf("Failed to create thread\n");
			exit(1);
		}

		idx++;
    
	}


	close(sockfd);
	pthread_mutex_destroy(&lock); 
	return 0;
}


