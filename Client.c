// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
   
#define PORT     8080
#define MAXLINE 1024

int main()
{
    int sockfd;
    char SentMsg[MAXLINE];
    char RecvMsg[MAXLINE];
    struct sockaddr_in     servaddr;
   
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;
       
    int n, len;

    while(1)
    {
        printf("Enter filename:\t");
        fgets(SentMsg, MAXLINE, stdin);
        SentMsg[strlen(SentMsg)-1]='\0';    //Remove newline
        printf("\n\n");
        
        sendto(sockfd, (const char *)SentMsg, strlen(SentMsg), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            
        n = recvfrom(sockfd, (char *)RecvMsg, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len);
        RecvMsg[n] = '\0';
        printf("%s\n", RecvMsg);

        memset(&SentMsg, 0, MAXLINE);   //We're preparing buffer for next request
        memset(&RecvMsg, 0, MAXLINE);   //We're preparing buffer for next request
    }
   
    close(sockfd);
    return 0;
}