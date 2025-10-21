#include "headers.h"


void HandleClient(char **argv)
{
    in_port_t serverPort = atoi(argv[2]);
    int serverSock;
    if((serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        fprintf(stderr, "Socket() failed.\n");
        exit(1);
    }

    int optval = 1;
    if(setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        fprintf(stderr, "setsockopt() failed.\n");
        close(serverSock);
        exit(1);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(serverPort);

    if(bind(serverSock, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) < 0)
    {
        fprintf(stderr, "bind() failed.\n");
        close(serverSock);
        exit(1);
    }

    if(listen(serverSock, MAXPENDING) < 0)
    {
        fprintf(stderr, "listen() failed.\n");
        close(serverSock);
        exit(1);
    }

    get_terminal_size();
    clear_screen();
    add_message("\t\t\t\t\tSERVER ACTIVE\n", 0, NULL);

    while(g_running)
    {
        struct sockaddr_in clntAddr;
        socklen_t clntAddrLen = sizeof(clntAddr);

        fd_set accept_fds;
        struct timeval timeout;

        FD_ZERO(&accept_fds);
        FD_SET(serverSock, &accept_fds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(serverSock + 1, &accept_fds, NULL, NULL, &timeout);

        if(activity < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            fprintf(stderr, "select() failed.\n");
            break;
        }

        if(activity == 0)
        {
            continue;
        }

        int clntSock = accept(serverSock, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if(clntSock < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            fprintf(stderr, "accept() failed.\n");
            continue;
        }
        
        char clntName[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL)
        {
            snprintf(msg_buff, sizeof(msg_buff), "\t\t\tHandling client %s on port %d\n\n", clntName, ntohs(serverAddr.sin_port));
            add_message(msg_buff, 0, NULL);
        }
        else
        {
            add_message("Unable to get client address\n", 0, NULL);
            strcpy(clntName, "unknown");
        }
            
        HandleTCPClient(clntSock, clntName);
    }
    close(serverSock);
}

void HandleTCPClient(int clntSock, char *clntName)
{
    char *Name = clntName; 
    char input[MAX_INPUT] = {0};
    size_t input_len = 0;
    size_t cursor_pos = 0;
    const size_t prompt_len = 4;

    setup_input_line(input);
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    while(g_running)
    {
        char servStr[BUFSIZE];
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(clntSock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = (clntSock > STDIN_FILENO) ? clntSock : STDIN_FILENO;

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT; 

        int activity = select(max_fd +1, &read_fds, NULL, NULL, &timeout);
        if(activity < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            add_message("select() error.\n", 0, NULL);
            break;
        }

        if(FD_ISSET(clntSock, &read_fds))
        {
            identifier = 2;

            char buffer[BUFSIZE];
            memset(buffer, 0, sizeof(buffer));
            ssize_t numBytesRcvd = recv(clntSock, buffer, (BUFSIZE - 1), 0);
            if(numBytesRcvd <= 0)
            {
                snprintf(msg_buff, sizeof(msg_buff), "recv() failed/ %s DISCONNECTED.\n", clntName);
                add_message(msg_buff, 0, NULL);
                break;
            }
            buffer[numBytesRcvd] = '\0';
            if(strcmp(buffer, "FILE_RECV") == 0)
            {
                const char ack[] = "ACK";
                if(send(clntSock, ack, strlen(ack), 0) < 0)
                {
                    add_message("Failed to send ACK\n", 0, NULL);
                    break;
                }

                if(rcvSocket(clntSock) == 0)
                {
                    add_message("FILE RECEIVED SUCCESSFULLY", 0, Name);
                }
                else
                {
                    add_message("FILE TRANSFER FAILED", 0, Name);
                }
            }
            else
            {
                add_message(buffer, identifier, Name);
            }
            setup_input_line(input);
            fflush(stdout);
        }

        if(FD_ISSET(STDIN_FILENO, &read_fds))
        {
            int key = read_key();
            
            if(key == '\n' || key == '\r')
            {
                if(input_len > 0)
                {
                    input[input_len] = '\0';
                    
                    identifier = 1;
                    add_message(input, identifier, Name);
                    
                    strncpy(servStr, input, sizeof(servStr) - 1);
                    servStr[sizeof(servStr) - 1] = '\0';
                    
                    const char *command_prefix = "::send";
                    size_t prefix_len = strlen(command_prefix);

                    if(strncmp(servStr, command_prefix, prefix_len) == 0)
                    {
                        const char protocol[] = "FILE_RECV";
                        size_t protocolLen = strlen(protocol);
                        ssize_t bytesSent = send(clntSock, protocol, protocolLen, 0);
                        
                        if(bytesSent < 0 || (size_t)bytesSent != protocolLen)
                        {
                            add_message("send() for file transfer failed.\n", 0, NULL);
                        }
                        else
                        {
                            char ack[4] = {0};
                            ssize_t ack_received = recv_with_timeout(clntSock, ack, 3, ACK_TIMEOUT);
                            
                            if(ack_received <= 0)
                            {
                                add_message("Failed to receive ACK from client (timeout or error).\n", 0, NULL);
                            }
                            else if(handle_user_input(servStr, clntSock, prefix_len) != 0)
                            {
                                add_message("File transfer encountered an error.\n", 0, NULL);
                            }
                        }
                    }
                    else
                    {
                        size_t servStrLen = strlen(servStr);
                        ssize_t numBytesSent = send(clntSock, servStr, servStrLen, 0);
                        
                        if(numBytesSent < 0)
                        {
                            add_message("send() failed\n", 0, NULL);
                            break;
                        }
                    }

                    memset(input, 0, sizeof(input));
                    input_len = 0;
                    cursor_pos = 0;
                }
                setup_input_line(input);
            }

            else if(key == 127 || key == '\b')
            {
                if(cursor_pos > 0)
                {
                    memmove(&input[cursor_pos - 1], &input[cursor_pos], input_len - cursor_pos + 1);
                    cursor_pos--;
                    input_len--;
                    setup_input_line(input);

                    printf("\x1b[%dG", (int)(prompt_len + cursor_pos + 1));
                    fflush(stdout);
                }
            }


            
            else if(key == 1003)
            {
                if(cursor_pos > 0)
                {
                    cursor_pos--;
                    printf("\x1b[D");
                    fflush(stdout);
                }
            }
            else if(key == 1002)
            {
                if(cursor_pos < input_len)
                {
                    cursor_pos++;
                    printf("\x1b[C");
                    fflush(stdout);
                }
            }



            else if(key >= 32 && key <= 126)
            {
                if(input_len < MAX_INPUT - 1)
                {
                    memmove(&input[cursor_pos + 1], &input[cursor_pos], input_len - cursor_pos + 1);
                    input[cursor_pos] = key;
                    cursor_pos++;
                    input_len++;
                    input[input_len] = '\0';
                    setup_input_line(input);

                    printf("\x1b[%dG", (int)(prompt_len + cursor_pos + 1));
                    fflush(stdout);
                }
            }
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    move_cursor(term_height + 1, 1);
    printf("\n");
    close(clntSock);
}
