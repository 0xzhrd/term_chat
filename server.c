#include "headers.h"
//this is to HANDLE THE SERVERS CONNECTIONS

void HandleServer(int argc, char *argv[])
{

    char clntStr[3000];
    char *ServerIP = argv[2];
    in_port_t ServerPort = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if(sock < 0)
    {
        printf("socket initialization failed\n");
        exit(1);
    }

    struct sockaddr_in ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;

    int rtnVal = inet_pton(AF_INET, ServerIP, &ServerAddr.sin_addr.s_addr);
    if(rtnVal == 0)
    {
        printf("inet_pton failed, invalid address string\n");
        exit(1);
    }
    else if(rtnVal < 0)
    {
        printf("inet_pton failed\n");
        exit(1);
    }
    ServerAddr.sin_port = htons(ServerPort);

    if(connect(sock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
    {
        printf("connect failed\n");
        exit(1);
    }

    memset(clntStr, 0, sizeof(clntStr));
    ssize_t numBytes;

    while(1)
    {
        printf(ANSI_BG_GREEN"[ME]: ");
        if(fgets(clntStr, sizeof(clntStr), stdin) != NULL)
        {
            size_t i  = sizeof(clntStr);
            if(i > 0 && clntStr[i - 1] == '\n')
            {
                clntStr[i - 1] = '\0';
            }
        }
        printf(ANSI_RESET);

        size_t clntStrLen = strlen(clntStr);
        numBytes = send(sock, clntStr, clntStrLen, 0);

        char buffer[BUFSIZE];
        numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
        if(numBytes < 0)
        {
            printf("recv() failed\n");
            close(sock);
            exit(1);
        } 
        else if(numBytes == 0)
        {
            printf("recv() connection closed prematurely");
            close(sock);
            exit(1);
        }
        buffer[numBytes] = '\0';

        char servName[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &ServerAddr.sin_addr.s_addr, servName, sizeof(servName)) == NULL)
            printf("Unable to get server address");
        printf(ANSI_BG_CYAN"[%s]: %s"ANSI_RESET, servName, buffer);
        memset(buffer, 0, sizeof(buffer));
    }
}
