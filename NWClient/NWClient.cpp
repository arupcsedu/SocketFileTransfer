/*  File Name : NWClient.cpp
    Author : Arup Sarker
    Creation date : 12/28/2020
    Description: Main responsibilities of this source are
    1. Create socket and connect with server to a specific port
    2. Send multiple files to server concurrently from a directory
    3. Utility functions to check file integrity by calculating checkSum

    Notes: BSD socket apis are used and compiled with g++ in Ubuntu 18.04. For Other OS, files needs be cross compiled and create executable
*/


#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h> 


#define MAX_LENGTH_OF_SINGLE_LINE 4096
#define SERVER_PORT 5080
#define BUFFER_SIZE 4096
#define MAX_FILE_COUNT 1000

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
typedef struct Threadparam {
	char fileName[BUFFER_SIZE];
	char filePath[BUFFER_SIZE];
	char serverIP[20];
} TP;

pthread_t tid[MAX_FILE_COUNT];

unsigned calculateChecksum(void *buffer, size_t len)
{
	unsigned char *buf = (unsigned char *)buffer;
	size_t i;
	unsigned int checkSum = 0;

	for (i = 0; i < len; ++i)
	    checkSum += (unsigned int)(*buf++);
	return checkSum;
}

unsigned int fileSender(FILE *filePointer, int sockfd) {
	// 13.1 Declare the data size
	int maxLen;

	// 13.2 Declare and Initalize the buffer for a single file
	char lineBuffer[MAX_LENGTH_OF_SINGLE_LINE] = {0};
	unsigned int checkSum = 0;

	// 13.3 Read the data from file in a loop
	while ((maxLen = fread(lineBuffer, sizeof(char), MAX_LENGTH_OF_SINGLE_LINE, filePointer)) > 0) {
	if (maxLen != MAX_LENGTH_OF_SINGLE_LINE && ferror(filePointer)) {
	    perror("Unable to read file");
	    exit(1);
	}
	checkSum += calculateChecksum(lineBuffer, maxLen);
	// 13.4 Send each line from buffer through TCP socket
	if (send(sockfd, lineBuffer, maxLen,0) == -1) {
	    perror("Unable to send file");
	    exit(1);
	}
	memset(lineBuffer, 0, MAX_LENGTH_OF_SINGLE_LINE);
	}
	return checkSum;
}
// ToDo : Add checkSum calculation method

void *threadForFileTransfer(void *vargp) 
{ 
	TP *sockParams = (TP *) vargp;

	if(sockParams == NULL) {
		printf("Socket Param Error");
		exit(1);
	}
	

	// 2. Create TCP Socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Unable to create Socket");
		exit(1);
	}

	// 3. Setup information about server
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));

	// 4. IP address should be IPv4
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);

	// 5. Check IP adddres and convert it with inet_pton
	if (inet_pton(AF_INET, sockParams->serverIP, &serverAddress.sin_addr) < 0) {
		perror("Conversion Error in IP Address");
		exit(1);
	}

	// 6. Client will connect after server bind
	if (connect(sockfd, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		perror("Unable to Connect");
		exit(1);
	}
	pthread_mutex_lock(&lock);
	// 11. Send file name from buffer data through socket so that server can create file with same name.
	

	if (send(sockfd, sockParams->fileName, BUFFER_SIZE, 0) == -1) {
		perror("Unable to send Filename");
		exit(1);
	}
	pthread_mutex_unlock(&lock);
	sleep(1);
	// 12. Create File pointers


	FILE *filePointer = fopen(sockParams->filePath, "rb");
	if (filePointer == NULL) {
		perror("Unable to open the file");
		exit(1);
	}
	unsigned int checkSum = 0;
	// 13. Send file through socket.
	pthread_mutex_lock(&lock);
	checkSum = fileSender(filePointer, sockfd);
	printf("%s file is sent successfully.\n",sockParams->fileName);
	pthread_mutex_unlock(&lock);
	sleep(1);

 
	char numberstring[BUFFER_SIZE];
	sprintf(numberstring, "%d", checkSum);
	printf("CheckSum is: %s \n", (char*)numberstring);

	//pthread_mutex_lock(&lock);
	if (send(sockfd, numberstring, BUFFER_SIZE, 0) == -1) {
		perror("Unable to send CheckSum");
		exit(1);
	}
	//pthread_mutex_unlock(&lock);
	//sleep(1);

	// 14. Close the file pointer
	fclose(filePointer);


	// 15. Close socket after sending single file.
	close(sockfd);
	pthread_exit(NULL);
} 
void sendConcurrencyNumber(char *ipAddr, int noOfconcurrency) {
	// 1. Utility functions for sending concurrency
	

	// 2. Create TCP Socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Unable to create Socket");
		exit(1);
	}

	// 3. Setup information about server
	struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));

	// 4. IP address should be IPv4
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(SERVER_PORT);

	// 5. Check IP adddres and convert it with inet_pton
	if (inet_pton(AF_INET, ipAddr, &serverAddress.sin_addr) < 0) {
		perror("Conversion Error in IP Address");
		exit(1);
	}

	// 6. Client will connect after server bind
	if (connect(sockfd, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
		perror("Unable to Connect");
		exit(1);
	}

 
	char numberstring[BUFFER_SIZE];
	sprintf(numberstring, "%d", noOfconcurrency);

	if (send(sockfd, numberstring, BUFFER_SIZE, 0) == -1) {
		perror("Unable to send Concurrency");
		exit(1);
	}

	close(sockfd);

}


int main(int argc, char* argv[]) {
	// 1. Check the parameters from command line argument
	if (argc < 3) {
		perror("IP address is missing");
		exit(1);
	}
	if (argc < 4) {
		perror("Concurrency argument is missing");
		exit(1);
	}
	
	if (pthread_mutex_init(&lock, NULL) != 0) {
        	printf("mutex init failed");
        	return 1;
    	}
	// 2. Take file name input from command line argument. This will be changed when we will read files from directory
	// 3. ToDo : Replace with directory path and read all the files from directory
	// Status : Done

	char *dirName = basename(argv[1]);
	if (dirName == NULL) {
		perror("Unable to get dirName");
		exit(1);
	}

	// 4. Read All the file list from directory

	DIR *dirPtr = opendir (dirName);

	// 5. validate directory open
	if (!dirPtr) {
		char errbuf[PATH_MAX] = "";
		sprintf (errbuf, "opendir failed on '%s'", dirName);
		perror (errbuf);
		return EXIT_FAILURE;
	}
	printf("Directory name : %s\n", dirName);

	struct dirent *dir;
	TP *params = NULL; 
	int idx = 0;
	char filePathList[MAX_FILE_COUNT][BUFFER_SIZE] = {0};
	char fileNameList[MAX_FILE_COUNT][BUFFER_SIZE] = {0};

	if(dirPtr!=NULL) {
		int flag = 0;

		while((dir=readdir(dirPtr))!=NULL) {
			if((strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0 || (*dir->d_name) == '.' )){
			}
			else
			{
				// 6. Define and Initialize the Buffer
				// 7. ToDo :  Add Concurrency for sending multiple files
				//Status : Done
				char buff[BUFFER_SIZE] = {0};
				char fileWithPath[BUFFER_SIZE] = {0};

				strcat(fileWithPath, dirName);
				strcat(fileWithPath, "/");

				strcat(fileWithPath, dir->d_name);
				// 8. Copy the file name into Buffer
				strncpy(buff, dir->d_name, strlen(dir->d_name));
				strcpy(fileNameList[idx], buff);
				strcpy(filePathList[idx], fileWithPath);

				printf("%s\n", filePathList[idx]);
				idx++;
			}
		}
	}
	
	// 9. Send Concurrency Number
	char *numstr = basename(argv[3]);
	int val = atoi(numstr);
	// 10. if number of files in a directory is less than concurrent number, replace it.
	if(val > idx) {
		printf("Maximum possible number of concurrency: %d\n", idx);	
		sendConcurrencyNumber(argv[2], idx);
	}
	else {
		idx = val;
		sendConcurrencyNumber(argv[2], idx);	
	}

	int i = 0;
	while(i != idx)
	{
		char *filename = (char*) malloc(sizeof(char));
		params = (TP*) malloc(sizeof(TP));
		strcpy(params->fileName, fileNameList[i]); 
		strcpy(params->filePath, filePathList[i]);
		strcpy(params->serverIP, argv[2]);
		// 11. Create multi-thread for each files
		if(pthread_create(&tid[i], NULL, threadForFileTransfer, (void*)params) != 0 ) {
			printf("Failed to create thread\n");
			exit(1);
		}
	
		printf("Thread creation : %d\n", i);
		i++;
	}
	sleep(20);
	i = 0;
	// 12. Execute Thread
	while(i != idx)
	{
		pthread_join(tid[i++],NULL);
		printf("%d:\n",i);
	}
	closedir (dirPtr);
	pthread_mutex_destroy(&lock); 
	return 0;
}


