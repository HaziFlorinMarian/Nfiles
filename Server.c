// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/stat.h>   //Used to get file metadata
#include <time.h>       //Used to format time-based metadata
   
#define PORT     8080
#define MAXLINE 1024

void GetFileMetadata(const char* szPath, char* Out)
{
    if (access(szPath, F_OK) != 0 )
        snprintf(Out, MAXLINE, "File \"%s\" does not exists.\n", szPath);
    else
    {
        struct stat res;
        stat(szPath, &res);

        int length = 0;
        length += snprintf(Out+length, MAXLINE, "Filename:\t\t%s\n", szPath);
        length += snprintf(Out+length, MAXLINE, "Filesize:\t\t%d\n", res.st_size);
        length += snprintf(Out+length, MAXLINE, "Permissions:\t\t");
        length += snprintf(Out+length, MAXLINE, (S_ISDIR(res.st_mode)) ? "d" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IRUSR) ? "r" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IWUSR) ? "w" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IXUSR) ? "x" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IRGRP) ? "r" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IWGRP) ? "w" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IXGRP) ? "x" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IROTH) ? "r" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IWOTH) ? "w" : "-");
        length += snprintf(Out+length, MAXLINE, (res.st_mode & S_IXOTH) ? "x\n" : "-\n");
        length += snprintf(Out+length, MAXLINE, "Owner:\t\t\t%d\n", res.st_gid);
        length += snprintf(Out+length, MAXLINE, "Last status change:\t%s", ctime(&res.st_ctime));
        length += snprintf(Out+length, MAXLINE, "Last file access:\t%s", ctime(&res.st_atime));
        length += snprintf(Out+length, MAXLINE, "Last file modification:\t%s", ctime(&res.st_mtime));
    }
}

int main()
{
    int sockfd;
    char InMsg[MAXLINE], OutMsg[MAXLINE];

    struct sockaddr_in servaddr, cliaddr;
       
    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
       
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
       
    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
       
    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int len, n;
   
    len = sizeof(cliaddr);  //len is value/resuslt

    while(1)
    {
        n = recvfrom(sockfd, (char *)InMsg, MAXLINE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
        InMsg[n] = '\0';
        printf("Client request: %s\n", InMsg);
        GetFileMetadata(InMsg, OutMsg);
        sendto(sockfd, (const char *)OutMsg, strlen(OutMsg),  MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len);

        memset(&InMsg, 0, MAXLINE);      //We're preparing buffer for next request
        memset(&OutMsg, 0, MAXLINE);     //We're preparing buffer for next request
    }
       
    return 0;
}