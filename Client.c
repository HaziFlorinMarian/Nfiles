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
#define MAX_PACKET_SIZE 65507 // The correct maximum UDP message size is 65507, as determined by the following formula:
                      // 0xffff - (sizeof(IP Header) + sizeof(UDP Header)) = 65535-(20+8) = 65507
#define MAX_BUFF_MSG (MAX_PACKET_SIZE - sizeof(int))
#pragma pack(1) // To align struct correctly, without losing 3 bytes
struct SPacket {
    int len;
    char msg[MAX_BUFF_MSG];
};

#define RED   "\x1B[31m"
#define RESET "\x1B[0m"

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
    
    char c, ans = getchar();

    while((c= getchar()) != '\n' && c != EOF) {}  // Remove any unwanted character from stdin

    if (ans == 'y' || ans == 'Y')
    {
        if (*ipCount >= maxLines)
        {
            printf(RED "ERROR! Maximum number of IP addresses has been reached!\n" RESET);
            return false;
        }

        FILE *fp = fopen("Servers.txt", "a");

        if (fp == NULL)
        {
            printf(RED "Can't open file Servers.txt!\n" RESET);
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
                printf(RED "ERROR! %s is not a valid IP address!" RESET, IP);
        } while (is_ok != true);

        fclose(fp);
        (*ipCount) += 1;
        
        while((c= getchar()) != '\n' && c != EOF) {}  // Remove any unwanted character from stdin

        return true;
    }

    return false;
}

int main()
{
    int sockfd, ipCount = 0, selIp = 0;
    struct SPacket toSend, toRecv;
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
       
    int len;

    //If we don't receive any answer in less than 1 second then is a problem..
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
        perror("Error");
    }

    while(1)
    {
        printf("Enter filename:\t");
        fgets(toSend.msg, MAX_BUFF_MSG, stdin);
        toSend.msg[strcspn(toSend.msg, "\n")] = '\0';    //Remove newline
        toSend.len = strlen(toSend.msg);
        
        if (toSend.len == 0)
            continue;

        if (strcmp(toSend.msg, "--exit") == 0)  // We're done
            break;

        int counter = 1, foundValidServer = 0;

        while (counter <= ipCount && foundValidServer == 0)
        {
            if(sendto(sockfd, &toSend, sizeof(toSend), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr)) <= 0)
            {
                perror (RED "Error! Cannot send message!\n" RESET);
                break;
            }

            if (recvfrom(sockfd, &toRecv, sizeof(toRecv), MSG_WAITALL, (struct sockaddr *) &servaddr, &len) > 0)
            {
                if (strcmp(toSend.msg, toRecv.msg) == 0)
                    AskAppendNewServer(ipList, 255, 16, &ipCount);
                else
                {
                    if (toRecv.len == strlen(toRecv.msg))
                    {
                        printf("%s\n", toRecv.msg);

                        if (toRecv.len == MAX_BUFF_MSG)
                            printf(RED "WARNING! THIS PACKET HAS REACHED MAXIMUM ALLOWED SIZE! IT MAY BE INCOMPLETE!!!\n\n" RESET);
                    }
                    else
                        printf("ERROR! Transmission eror (expected %d bytes and received %d.\n", toRecv.len, strlen(toRecv.msg));
                    }

                    foundValidServer = 1;
            }
            else
            {
                printf(RED "Timeout reached :: server (%s) not available, trying next server available (%s)\n" RESET, ipList[selIp], ipList[(selIp+1) % ipCount]);
                counter += 1;
                selIp = (selIp+1) % ipCount;
                servaddr.sin_addr.s_addr = inet_addr(ipList[selIp]); // Try next available server
            }
        }
            
        if (foundValidServer == false)
        {
            printf(RED "All servers are unavailable. Please try again later.\n" RESET);
            break;
        }

        memset(&toSend, 0, MAX_PACKET_SIZE);   //We're preparing buffer for next request
        memset(&toRecv, 0, MAX_PACKET_SIZE);   //We're preparing buffer for next request
    }

    for (int i = 0; i < ipCount; ++i)
        free(ipList[i]); //No memory leaks, please.
    close(sockfd);
    return 0;
}