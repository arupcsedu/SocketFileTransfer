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

#define MAX_LENGTH_OF_SINGLE_LINE 4096

#define LISTEN_PORT 8050

#define SERVER_PORT 5080

#define BUFFER_SIZE 4096


// This is a utility function for writing data into file

void fileWriter(int sockfd, FILE *fp) {
    // 11.1 Declare data length
    ssize_t n;

    // 11.2 Declare and Initialize buffer size
    char buff[MAX_LENGTH_OF_SINGLE_LINE] = {0};
    n = read(sockfd, buff, MAX_LENGTH_OF_SINGLE_LINE);

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
        memset(buff, 0, MAX_LENGTH_OF_SINGLE_LINE);
    }
    printf("End of File Writer\n");
}

int main(int argc, char *argv[]) {
    // 1. Create TCP Socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Unable to create Socket");
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

          // 6. Accept connection
    socklen_t addrlen = sizeof(nwClientAddr);
    int connfd = accept(sockfd, (struct sockaddr *) &nwClientAddr, &addrlen);
    if (connfd == -1) {
        perror("Error in Accepting Connection");
        exit(1);
    }

    int fileCount = 0;
    while(1) {

        // 8. Receive incoming file from client
        char filename[BUFFER_SIZE] = {0};
        if (read(connfd, filename, BUFFER_SIZE) == -1) {
            perror("Unable to receive file due to connection or file error");
            exit(1);
        }

        if(strlen(filename) > 0) {
            // 9. After receiving from client, create file to save data

            fileCount++;
            printf("%d. File Name : %s\n", fileCount, filename);
            FILE *fp = fopen(filename, "wb");
            if (fp == NULL) {
                perror("Unable to create File pointer");
                exit(1);
            }

            // 10. Start receiving file Data and print in console
            char addr[INET_ADDRSTRLEN];
            printf("File is receiving: %s from %s\n", filename, inet_ntop(AF_INET, &nwClientAddr.sin_addr, addr, INET_ADDRSTRLEN));

            // 11. Write the data into file.
            fileWriter(connfd, fp);

            // 12. Print success message into console
            printf("Received file number : %d\n",fileCount);


            // 13. Send Acknowledgement of first received file.
            if (write(connfd, "ACK", BUFFER_SIZE) == -1) {
                perror("Unable to send Filename");
                exit(1);
            }
            else {
                printf("ACK is sent for file: %d\n\n\n", fileCount);
            }


             // 14. Close the file pointers as well as socket.
             // 15. ToDo: Set the condition for receiving maximum 10 files sequencially. It will changed during the implementation of concurrency.
            if(fileCount == 10) {
                fclose(fp);
                close(connfd);
                close(sockfd);
                break;
            }
        }

    }

    return 0;
}


