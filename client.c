#include "headers.h"
//this is to START A SERVER and handle client connections 

void HandleClient(int argc, char **argv)
{
    in_port_t serverPort = atoi(argv[2]);
    int serverSock;
    if((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("Socket() failed.\n");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);

    if(bind(serverSock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("bind() failed.\n");
        close(serverSock);
        exit(1);
    }

    if(listen(serverSock, MAXPENDING) < 0)
    {
        printf("listen() failed.\n");
        close(serverSock);
        exit(1);
    }

    for(;;)
    {
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);

        int clntSock = accept(serverSock, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if(clntSock < 0)
        {
            printf("accept() failed.\n");
            close(serverSock);
            exit(1);
        }
        
        char clntName[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL)
            printf("\t\t\tHandling client %s on port %d\n\n", clntName, ntohs(serverAddr.sin_port));
        else
            puts("Unable to get client address\n");
            
        HandleTCPClient(clntSock, clntName);
    }

}


void HandleTCPClient(int clntSocket, char *clntName)
{
    char buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);
    ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
    if(numBytesRcvd < 0)
    {
        printf("recv() failed\n");
        exit(1);
    }

    while(numBytesRcvd > 0)
    {
        printf(ANSI_BG_GREEN"[%s]: %s"ANSI_RESET, clntName, buffer);
        memset(buffer, 0 , sizeof(buffer));

        printf(ANSI_BG_CYAN"[ME]: ");
        if(fgets(servStr, sizeof(servStr), stdin) != NULL)
        {
            size_t i  = sizeof(servStr);
            if(i > 0 && servStr[i - 1] == '\n')
            {
                servStr[i - 1] = '\0';
            }
        }
        printf(ANSI_RESET);

        size_t servStrLen = strlen(servStr);
        ssize_t numBytesSent = send(clntSocket, servStr, servStrLen, 0);
        if(numBytesSent < 0)
        {
            printf("send() failed\n");
            close(clntSocket);
            exit(1);
        }
        memset(servStr,0, sizeof(servStr));

        numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);      
        if(numBytesRcvd < 0)
        {
            printf("recv(): failed");
            close(clntSocket);
            exit(1);
        }
    }
}