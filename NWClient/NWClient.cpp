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

#define MAX_LENGTH_OF_SINGLE_LINE 4096

#define SERVER_PORT 5080

#define BUFFER_SIZE 4096

void fileSender(FILE *filePointer, int sockfd) {
    // 13.1 Declare the data size
    int maxLen;

    // 13.2 Declare and Initalize the buffer for a single file
    char lineBuffer[MAX_LENGTH_OF_SINGLE_LINE] = {0};

    // 13.3 Read the data from file in a loop
    while ((maxLen = fread(lineBuffer, sizeof(char), MAX_LENGTH_OF_SINGLE_LINE, filePointer)) > 0) {
        if (maxLen != MAX_LENGTH_OF_SINGLE_LINE && ferror(filePointer)) {
            perror("Unable to read file");
            exit(1);
        }

        // 13.4 Send each line from buffer through TCP socket
        if (write(sockfd, lineBuffer, maxLen) == -1) {
            perror("Unable to send file");
            exit(1);
        }
        memset(lineBuffer, 0, MAX_LENGTH_OF_SINGLE_LINE);
    }
}
// ToDo : Add checkSum calculation method

int main(int argc, char* argv[]) {
    // 1. Check the parameters from command line argument
    if (argc != 3) {
        perror("IP address is missing");
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
    if (inet_pton(AF_INET, argv[2], &serverAddress.sin_addr) < 0) {
        perror("Conversion Error in IP Address");
        exit(1);
    }

    // 6. Client will connect after server bind
    if (connect(sockfd, (const struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("Unable to Connect");
        exit(1);
    }

    // 7. Take file name input from command line argument. This will be changed when we will read files from directory
    // 8. ToDo : Replace with directory path and read all the files from directory
    // Status : Done

    char *dirName = basename(argv[1]);
    if (dirName == NULL) {
        perror("Unable to get dirName");
        exit(1);
    }

    // Read All the file list from directory

    DIR *dirPtr = opendir (dirName);

    // validate directory open
    if (!dirPtr) {
        char errbuf[PATH_MAX] = "";
        sprintf (errbuf, "opendir failed on '%s'", dirName);
        perror (errbuf);
        return EXIT_FAILURE;
    }
    printf("Directory name : %s\n", dirName);

    struct dirent *dir;

    if(dirPtr!=NULL) {
    int flag = 0;
        while((dir=readdir(dirPtr))!=NULL) {
            if((strcmp(dir->d_name,".")==0 || strcmp(dir->d_name,"..")==0 || (*dir->d_name) == '.' )){
            }
            else
            {
                // 9. Define and Initialize the Buffer
                // ToDo :  Add Concurrency for sending multiple files
                char buff[BUFFER_SIZE] = {0};
                char fileWithPath[BUFFER_SIZE] = {0};

                strcat(fileWithPath, dirName);
                strcat(fileWithPath, "/");

                strcat(fileWithPath, dir->d_name);
                // 10. Copy the file name into Buffer
                strncpy(buff, dir->d_name, strlen(dir->d_name));

                printf("%s\n",buff);

                // 11. Send file name from buffer data through socket so that server can create file with same name.
                char ackBuff[MAX_LENGTH_OF_SINGLE_LINE] = {0};
                int noOfLines;
                if(flag)
                     noOfLines = read(sockfd, ackBuff, MAX_LENGTH_OF_SINGLE_LINE);

                printf("Size : %d  Received: %s\n", noOfLines, ackBuff);
                if((noOfLines > 0 && strlen(ackBuff) > 1) || flag == 0) {


                    if (write(sockfd, buff, BUFFER_SIZE) == -1) {
                        perror("Unable to send Filename");
                        exit(1);
                    }
                    flag = 1;

                    // 12. Create File pointers

                    FILE *filePointer = fopen(fileWithPath, "rb");
                    if (filePointer == NULL) {
                        perror("Unable to open the file");
                        exit(1);
                    }

                    // 13. Send file through socket.
                    fileSender(filePointer, sockfd);
                    printf("File is sent successfully.\n");

                    // 14. Close the file pointer
                    fclose(filePointer);
                }
            }
        }
    }

    closedir (dirPtr);

    // 15. Close socket after sending single file.
    // 16. ToDo: Keep Socket open until sending all files concurrently from a directory.
    close(sockfd);
    return 0;
}


