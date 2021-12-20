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
#include <dirent.h>     //User to list directory's content
   
#define PORT     8080
#define MAX_PACKET_SIZE 65507 // The correct maximum UDP message size is 65507, as determined by the following formula:
                      // 0xffff - (sizeof(IP Header) + sizeof(UDP Header)) = 65535-(20+8) = 65507
#define MAX_BUFF_MSG (MAX_PACKET_SIZE - sizeof(int))
#pragma pack(1) // To align struct correctly, without losing 3 bytes

struct SPacket {
    int len;
    char msg[MAX_BUFF_MSG];
};

void GetFileMetadata(const char* szPath, struct SPacket *Out)
{
    if (access(szPath, F_OK) != 0 )
        snprintf(Out->msg, MAX_BUFF_MSG, "%s", szPath);
    else
    {
        struct stat res;
        stat(szPath, &res);

        int length = 0;
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Filename:\t\t%s\n", szPath);
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Filesize:\t\t%d\n", res.st_size);
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Permissions:\t\t");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (S_ISDIR(res.st_mode)) ? "d" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IRUSR) ? "r" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IWUSR) ? "w" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IXUSR) ? "x" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IRGRP) ? "r" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IWGRP) ? "w" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IXGRP) ? "x" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IROTH) ? "r" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IWOTH) ? "w" : "-");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, (res.st_mode & S_IXOTH) ? "x\n" : "-\n");
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Owner:\t\t\t%d\n", res.st_gid);
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Last status change:\t%s", ctime(&res.st_ctime));
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Last file access:\t%s", ctime(&res.st_atime));
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Last file modification:\t%s", ctime(&res.st_mtime));

        DIR *d;
        struct dirent *dir;
        d = opendir(szPath);
        if (d != NULL)
        {
            length += snprintf(Out->msg+length, MAX_BUFF_MSG, "Content of %s:\t", szPath);
            while ((dir = readdir(d)) != NULL)
            {
                length += snprintf(Out->msg+length, MAX_BUFF_MSG, "%s\t", dir->d_name);
            }

            closedir(d);
        }
        length += snprintf(Out->msg+length, MAX_BUFF_MSG, "\n");
        Out->len = length;
    }
}

int main()
{
    int sockfd;
    struct SPacket toSend, toRecv;

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

    int len;
   
    len = sizeof(cliaddr);  //len is value/resuslt

    while(1)
    {
        if (recvfrom(sockfd, &toRecv, sizeof(toRecv), MSG_WAITALL, (struct sockaddr *) &servaddr, &len) <= 0)
        {
            perror ("Error! Cannot read message!\n");
            break;
	    }

        toRecv.msg[toRecv.len] = '\0';
        printf("[DEBUG] Client request: %s\n", toRecv.msg);
        GetFileMetadata(toRecv.msg, &toSend);
        printf("[DEBUG] Server answer: %s (len = %d)\n", toSend.msg, toSend.len);

        toSend.msg[toSend.len - 1] = '\n';

        if (sendto(sockfd, &toSend, sizeof(toSend), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)) <= 0)
        {
            perror ("Error! Cannot send message!\n");
            break;
	    }

        memset(&toSend, 0, MAX_PACKET_SIZE);      //We're preparing buffer for next request
        memset(&toRecv, 0, MAX_PACKET_SIZE);     //We're preparing buffer for next request
    }
       
    return 0;
}