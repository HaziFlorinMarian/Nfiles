// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h> //So retarded, need this for bool functions because we compile with C99 standard
   
#define PORT     8080
#define MAXLINE 65527 // 65535 is MAX and other 8 bytes are reserved for header

struct SPacket {
    int len;
    char msg[MAXLINE - sizeof(int)];
};

bool isValidIpAddress(char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr)); //Convert IP address from string to binary
    return result != 0;
}

bool ReadIPsList(char** iplist, int maxLines, int maxLen, int* ipCount)
{
    FILE *fp = fopen("Servers.txt", "r");
    size_t len = 255;

    char *line = malloc(sizeof(char) * len);

    if (fp == NULL)
    {
        printf("Can't open file Servers.txt!\n");
        return false;
    }

    while(fgets(line, len, fp) != NULL)
    {
        line[strlen(line)-1]='\0';    //Remove newline

        if (*ipCount < maxLines && strlen(line) < maxLen && isValidIpAddress(line))
        {
            iplist[*ipCount] = (char *)malloc(16);
            strncpy(iplist[*ipCount], line, maxLen);
            printf("\"%s\" added to IP list\n", iplist[*ipCount]);
        }
        else
        {
            printf("Invalid IP address (%s), line (%d).\n", line, *ipCount);
            return false;
        }

        (*ipCount) += 1;
    }

    if (*ipCount == 0)
    {
        printf("Servers.txt is empty ?!?\n");
        return false;
    }

    fclose(fp);
    free(line);
    printf("\n");
    return true;
}

bool AskAppendNewServer(char** iplist, int maxLines, int maxLen, int* ipCount)
{
    printf("Do you want to add a new server? (Y/N)\n");
    
    char ans = getchar();

    if (ans == 'y' || ans == 'Y')
    {
        if (*ipCount >= maxLines)
        {
            printf("ERROR! Maximum number of IP addresses has been reached!\n");
            return false;
        }

        FILE *fp = fopen("Servers.txt", "a");

        if (fp == NULL)
        {
            printf("Can't open file Servers.txt!\n");
            return false;
        }


        bool is_ok = false;
        char IP[16 + 1];
        do {
            printf("\nEnter IP address: ");
            scanf("%s", IP);

            if (isValidIpAddress(IP))
            {
                is_ok = true;
                fprintf(fp, "%s\n", IP);

                iplist[*ipCount] = (char *)malloc(16);
                strncpy(iplist[*ipCount], IP, maxLen);
                printf("\"%s\" added to IP list\n", iplist[*ipCount]);
            }
            else
                printf("ERROR! %s is not a valid IP address!", IP);
        } while (is_ok != true);

        fclose(fp);
        (*ipCount) += 1;

        return true;
    }

    return false;
}

int main()
{
    int sockfd, ipCount = 0, selIp = 0;
    char SentMsg[MAXLINE];
    char RecvMsg[MAXLINE];
    struct sockaddr_in servaddr;

    char* ipList[255];
    if (ReadIPsList(ipList, 255, 16, &ipCount) == 0)
        return -1;
   
    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
   
    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(ipList[selIp]); //Trying 1st server then we move to a new one if this become unavailable
       
    int n, len;

    //If we don't receive any answer in less than 1 second then is a problem..
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    while(1)
    {
        char c;
        while((c= getchar()) != '\n' && c != EOF);  // Remove any unwanted character from stdin (scanf from AskAppendNewServer created this issue)
        printf("Enter filename:\t");
        fgets(SentMsg, MAXLINE, stdin);
        SentMsg[strcspn(SentMsg, "\n")] = '\0';    //Remove newline
        
        if (strlen(SentMsg) == 0)
            continue;
        
        sendto(sockfd, (const char *)SentMsg, strlen(SentMsg), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
            
        if (n = recvfrom(sockfd, (char *)RecvMsg, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len) > 0)
        {
            if (strcmp(SentMsg, RecvMsg) == 0) // Yep, this file does not exists on host.
            {
                if (AskAppendNewServer(ipList, 255, 16, &ipCount) == true)
                {
                    selIp = ipCount - 1; //We append always at the end of the array
                    printf("Server (%s) has been added successfully. Now you can send your request from it.\n", ipList[selIp]);
                    servaddr.sin_addr.s_addr = inet_addr(ipList[selIp]);
                }
            }
            else
                printf("%s\n", RecvMsg);
            // printf("[DEBUG] Received msg len: %d\n", strlen(RecvMsg));
        }
        else
        {
            int counter = 1, foundValidServer = 0;

            while (counter < ipCount && foundValidServer == 0)
            {
                printf("Timeout reached :: server (%s) not available, trying next server available (%s)\n", ipList[selIp], ipList[(selIp+1) % ipCount]);
                counter += 1;
                selIp = (selIp+1) % ipCount;
                servaddr.sin_addr.s_addr = inet_addr(ipList[selIp]); // Try next available server
                sendto(sockfd, (const char *)SentMsg, strlen(SentMsg), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

                if (n = recvfrom(sockfd, (char *)RecvMsg, MAXLINE, MSG_WAITALL, (struct sockaddr *) &servaddr, &len) > 0)
                {
                    if (strcmp(SentMsg, RecvMsg) == 0)
                        AskAppendNewServer(ipList, 255, 16, &ipCount);
                    else
                        printf("%s\n", RecvMsg);

                    foundValidServer = 1;
                }
            }
        }

        memset(&SentMsg, 0, MAXLINE);   //We're preparing buffer for next request
        memset(&RecvMsg, 0, MAXLINE);   //We're preparing buffer for next request
    }

    free(ipList); //No memory leaks, please.
    close(sockfd);
    return 0;
}