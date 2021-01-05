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

#define MAX_LENGTH_OF_SINGLE_LINE 4096

#define LISTEN_PORT 8050

#define SERVER_PORT 5080

#define BUFFER_SIZE 100000
#define MAX_NAME 1000
#define MAX_THREAD 4 
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

unsigned int fileWriter(int sockfd, FILE *fp) {
	// 11.1 Declare data length
	ssize_t n;
	// 11.2 Declare and Initialize buffer size
	unsigned int checkSum = 0;
	char buff[BUFFER_SIZE] = {0};
	n = recv(sockfd, buff, BUFFER_SIZE,0);

	checkSum += calculateChecksum(buff, n);

	// 11.3 Receive data from socket and save into buffer
	if(n > 0) {
		if (n == -1) {
			perror("Error in Receiving File");
			exit(1);
		}

		// 11.4 Read File Data from buffer and Write into file
		if (fwrite(buff, sizeof(char), n, fp) != n) {
			perror("Error in Writing File");
			exit(1);
		}

		printf("File content is : %s\n", buff);
		// Allocate buffer
		memset(buff, 0, BUFFER_SIZE);
	}
	printf("End of File Writer\n");
	fclose(fp);
	return checkSum;
}



void* socketThreadFT(void *arg)
{
	int connfd = *((int *)arg);
	//printf("Connection ID : %d\n", connfd);
	unsigned int checkSum = 0;

	pthread_mutex_lock(&lock);
	 // 8. Receive incoming file from client
	char filename[MAX_NAME] = {0};
	if (recv(connfd, filename, MAX_NAME,0) == -1) {
		perror("Unable to receive file due to connection or file error");
		exit(1);
	}

	int fileLength = strlen(filename);


	if(fileLength > 0) {
		// 9. After receiving from client, create file to save data

		printf("File Name : %s\n", filename);
		FILE *fp = fopen(filename, "wb");
		if (fp == NULL) {
			perror("Unable to create File pointer");
			exit(1);
		}

		// 10. Start receiving file Data and print in console
		char addr[INET_ADDRSTRLEN];

		// 11. Write the data into file.

		checkSum = fileWriter(connfd, fp);

		// 12. Print success message into console

		pthread_mutex_unlock(&lock);
		sleep(1);
		
		pthread_mutex_lock(&lock);
		// 13. Receive incoming checkSum from client
		char numberstring[MAX_NAME] = {0};
		if (recv(connfd, numberstring, MAX_NAME,0) == -1) {
			perror("Unable to receive file due to connection or file error");
			exit(1);
		}
		char checkSumstring[MAX_NAME];
		sprintf(checkSumstring, "%d", checkSum);
		printf("Calculated CheckSum: %s and Received CheckSum : %s\n", (char*)checkSumstring, (char*)numberstring);
		
		if(strcmp(checkSumstring, numberstring))
			printf("Error in receiving file.\n");
		else
			printf("Received file name : %s\n",filename);
		pthread_mutex_unlock(&lock);
		sleep(1);

			


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
	printf("Concurrency Number: %d\n", val);
	close(cfd);
	pthread_mutex_unlock(&lock);

	
	int i = 0;
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
		printf("Connection ID: %d\n", *new_sock);
		// 8. Multi-Thread creation
		if(pthread_create(&tid[idx], NULL, socketThreadFT, (void*)new_sock) < 0 ) {
			printf("Failed to create thread\n");
			exit(1);
		}
		idx++;	
		// 9. Thread Execute 
		if(idx == val) {
			idx = 0;
			while(idx != val) {
				pthread_join(tid[idx++],NULL);
			}
			i = 0;
		}
    
	}


	close(sockfd);
	pthread_mutex_destroy(&lock); 
	return 0;
}


