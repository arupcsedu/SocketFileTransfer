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
#include <assert.h>
#include <time.h>


#define MAX_LENGTH_OF_SINGLE_LINE 4096
#define SERVER_PORT 5080
#define BUFFER_SIZE 100000
#define MAX_FILE_COUNT 1000
#define MAX_NAME 1000

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
typedef struct Threadparam {
	char fileName[MAX_NAME];
	char filePath[MAX_NAME];
	char serverIP[20];
	bool isUsed;
} TP;

typedef struct SocketParams {
	TP *threadParamList[MAX_FILE_COUNT];
	int fileCount;
} SP;

pthread_t tid[MAX_FILE_COUNT];
TP *threadParamList[MAX_FILE_COUNT];

// ToDo : Add checkSum calculation method
// Status: Done
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


	// 13.2 Declare and Initalize the buffer for a single file

	char lineBuffer[BUFFER_SIZE] = {0};
	unsigned int checkSum = 0;
	int maxLen = 0;
	// 13.3 Read the data from file in a loop
	while ((maxLen = fread(lineBuffer, sizeof(char), BUFFER_SIZE, filePointer)) > 0) {
		if (maxLen != BUFFER_SIZE && ferror(filePointer)) {
		    perror("Unable to read file");
		    exit(1);
		}
		//printf("File is Sending to Socket : %d\n", sockfd);
		checkSum += calculateChecksum(lineBuffer, maxLen);
		
		// 13.4 Send each line from buffer through TCP socket
		if (send(sockfd, lineBuffer, maxLen,0) == -1) {
		    perror("Unable to send file");
		    exit(1);
		}
		memset(lineBuffer, '\0', BUFFER_SIZE);
	}
	fclose(filePointer);

	return checkSum;
}


void *threadForFileTransfer(void *vargp) 
{ 
	SP *sockParamList = (SP *) vargp;

	if(sockParamList == NULL) {
		printf("Socket Param Error");
		exit(1);
	}
	

	for(int i = 0; i < sockParamList->fileCount; i++) {

		if(!sockParamList->threadParamList[i]->isUsed) {
			pthread_mutex_lock(&lock);
			sockParamList->threadParamList[i]->isUsed = true;
			pthread_mutex_unlock(&lock);
			//sleep(1);
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
			if (inet_pton(AF_INET, sockParamList->threadParamList[0]->serverIP, &serverAddress.sin_addr) < 0) {
				perror("Conversion Error in IP Address");
				exit(1);
			}

			// 6. Client will connect after server bind
			if (connect(sockfd, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
				perror("Unable to Connect");
				exit(1);
			}
			// 11. Send file name from buffer data through socket so that server can create file with same name.
			
			time_t t = time(NULL);
			struct tm *tm = localtime(&t);
			char s[64];
			assert(strftime(s, sizeof(s), "%c", tm));

			printf("%s file Sending is started at : %s\n",sockParamList->threadParamList[i]->fileName,s);

			if (send(sockfd, sockParamList->threadParamList[i]->fileName, MAX_NAME, 0) == -1) {
				perror("Unable to send Filename");
				exit(1);
			}

			// 12. Create File pointers
			

			FILE *filePointer = fopen(sockParamList->threadParamList[i]->filePath, "rb");
			if (filePointer == NULL) {
				perror("Unable to open the file");
				exit(1);
			}
			unsigned int checkSum = 0;
			// 13. Send file through socket.
			checkSum = fileSender(filePointer, sockfd);
			
			struct tm *tm1 = localtime(&t);
			char endTime[64];
			assert(strftime(endTime, sizeof(endTime), "%c", tm1));
			printf("%s file is sent successfully with checkSum : %d at %s\n",sockParamList->threadParamList[i]->fileName, checkSum, endTime);
			close(sockfd);

		}
	}


	// 18. Close socket after sending single file.
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

 
	char numberstring[MAX_NAME];
	sprintf(numberstring, "%d", noOfconcurrency);

	if (send(sockfd, numberstring, MAX_NAME, 0) == -1) {
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

	int idx = 0;
	char filePathList[MAX_FILE_COUNT][MAX_NAME] = {0};
	char fileNameList[MAX_FILE_COUNT][MAX_NAME] = {0};
	SP *sParams = (SP*) malloc(sizeof(SP));
	if(dirPtr!=NULL) {
		int flag = 0;

		while((dir=readdir(dirPtr))!=NULL) {
			if((strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0 || (*dir->d_name) == '.' )){
			}
			else
			{
				// 6. Define and Initialize the Buffer
				// 7. ToDo :  Add Concurrency for sending multiple files
				// Status : Done

				char buff[MAX_NAME] = {0};
				char fileWithPath[MAX_NAME] = {0};

				strcat(fileWithPath, dirName);
				strcat(fileWithPath, "/");

				strcat(fileWithPath, dir->d_name);

				// 8. Copy the file name into Buffer
				strncpy(buff, dir->d_name, strlen(dir->d_name));
				
				TP *params = (TP*) malloc(sizeof(TP));

				strcpy(params->fileName, buff);
				strcpy(params->filePath, fileWithPath);
				strcpy(params->serverIP, argv[2]);
				params->isUsed = false;

				sParams->threadParamList[idx] = params;
				printf("%s\n", fileWithPath);
				idx++;
			}
		}
		sParams->fileCount = idx;
	}
	if(idx == 0) {
		printf("There is no file in the directory : %s\n", dirName);
		return 1;
	}
	
	// 9. Send Number file has to receive by NWServer
	char *numstr = basename(argv[3]);
	int val = atoi(numstr);

	// 10. if number of files in a directory is less than concurrent number, replace it.
	if(val > idx) {
		printf("Maximum possible number of concurrency: %d\n", val);	
		sendConcurrencyNumber(argv[2], idx);
	}
	else {
		//idx = val;
		sendConcurrencyNumber(argv[2], idx);	
	}

	int j = 0;
	while(j != val) {

		// 11. Create multi-thread for each files
		if(pthread_create(&tid[j++], NULL, threadForFileTransfer, (void*)sParams) != 0 ) {
			printf("Failed to create thread\n");
			exit(1);
		}
		printf("Thread ID : %d\n", j);
	}

	// 12. Execute Thread
	j = 0;
	while(j != val)
	{
		pthread_join(tid[j++],NULL);
		printf("pthread join %d:\n",j);

	}
	closedir (dirPtr);
	pthread_mutex_destroy(&lock); 
	return 0;
}


