#include "headers.h"

void HandleClient(char **argv)
{
    memset(centered, 0, BUFSIZE);

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
    save_term_state();
    clear_screen();

    format_system_messages(centered, sizeof(centered), "SERVER ACTIVE\n");
    add_message(centered, 0, NULL);
    memset(centered, 0, BUFSIZE);

    while(running)
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
            close(serverSock);
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
            snprintf(msg_buff, sizeof(msg_buff), "Handling client %s on port %d\n\n", clntName, ntohs(serverAddr.sin_port));
            format_system_messages(centered, sizeof(centered), msg_buff);
            add_message(centered, 0, NULL);
            memset(centered, 0, sizeof(centered));
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

    set_raw_mode();
    setup_input_line(input);

    while(running)
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
            memset(buffer, 0, BUFSIZE);
            ssize_t numBytesRcvd = recv(clntSock, buffer, (BUFSIZE - 1), 0);
            if(numBytesRcvd <= 0)
            {
                snprintf(msg_buff, sizeof(msg_buff), "recv() failed/ %s DISCONNECTED.\n", clntName);
                format_system_messages(centered, sizeof(centered), msg_buff);            
                add_message(centered, 0, NULL);
                memset(centered, 0, sizeof(centered));
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
                    format_system_messages(centered, sizeof(centered), "FILE RECEIVED SUCCESSFULLY.\n");
                    add_message(centered, 0, Name);
                    memset(centered, 0, sizeof(centered));
                }
                else
                {
                    format_system_messages(centered, sizeof(centered), "FILE TRANSFER FAILED.\n");
                    add_message(centered, 0, Name);
                    memset(centered, 0,  sizeof(centered));
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

            if(key == KEY_PAGE_UP)
            {
                scroll_offset += (term_height - 2);
                display_messages(&g_msg_buffer, Name);
                setup_input_line(input);
            }
            else if(key == KEY_PAGE_DOWN)
            {
                scroll_offset -= (term_height - 2);
                if(scroll_offset < 0) scroll_offset = 0;
                display_messages(&g_msg_buffer, Name);
                setup_input_line(input);
            }

            else if(key == '\n' || key == '\r')
            {
                if(input_len > 0)
                {
                    input[input_len] = '\0';

                    if(strcmp(input, "::quit") == 0)
                    {
                        add_message("\n\nDISCONNECTED.\n", 0, NULL);
                        memset(input, 0, sizeof(input));
                        restore_term_state();
                        move_cursor(term_height + 1, 1);
                        printf("\n");
                        exit(0);
                    }
                    
                    identifier = 1;
                    add_message(input, identifier, Name);
                    
                    strncpy(servStr, input, sizeof(servStr) - 1);
                    servStr[sizeof(servStr) - 1] = '\0';
                    
                    const char *command_prefix = "::send";
                    size_t prefix_len = strlen(command_prefix);

                    if(strncmp(servStr, command_prefix, prefix_len) == 0)
                    {
                        const char *filename_start = servStr + prefix_len;

                        if(*filename_start == '\0' || *filename_start == '\n')
                        {
                            format_system_messages(centered, sizeof(centered), "Error: No filename provided.\n");
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                            //
                            memset(input, 0, sizeof(input));
                            input_len = 0;
                            cursor_pos = 0;
                            setup_input_line(input);
                            continue;
                        }

                        const char protocol[] = "FILE_RECV";
                        size_t protocolLen = strlen(protocol);
                        ssize_t bytesSent = send(clntSock, protocol, protocolLen, 0);
                        
                        if(bytesSent < 0)
                        {
                            add_message("send() for file transfer failed.\n", 0, NULL);
                            //
                            perror("send");
                            memset(input, 0, sizeof(input));
                            input_len = 0;
                            cursor_pos = 0;
                            setup_input_line(input);
                            continue;
                        }
                        else if((size_t)bytesSent != protocolLen)
                        {
                            snprintf(msg_buff, sizeof(msg_buff), "Partial send of protocol message (%zd/%zu bytes).\n", bytesSent, protocolLen);
                            format_system_messages(centered, sizeof(centered), msg_buff);
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                            continue;
                        }
                        
                        char ack[4] = {0};
                        ssize_t ack_received = recv_with_timeout(clntSock, ack, 3, ACK_TIMEOUT);
                        
                        if(ack_received < 0)
                        {
                            format_system_messages(centered, sizeof(centered), "Failed to receive ACK (timeout/error).\n");
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                            continue;
                        }

                        else if(ack_received == 0)
                        {
                            format_system_messages(centered, sizeof(centered), "Server closed connection while waiting for ACK.\n");
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                            continue;
                        } 
                        else if(ack_received < 3)
                        {
                            format_system_messages(centered, sizeof(centered), "Incomplete ACK received from server.\n");
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                            continue;
                        }

                        else if(strncmp(ack, "ACK", 3) != 0)
                        {
                            format_system_messages(centered, sizeof(centered), "Invalid ACK received from server.\n");
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                            continue;
                        }
                        else if(handle_user_input(servStr, clntSock, prefix_len) != 0)
                        {
                            format_system_messages(centered, sizeof(centered), "File transfer encountered an error.\n");
                            add_message(centered, 0, NULL);
                            memset(centered, 0, sizeof(centered));
                        }
                        continue; 
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
            else if(key == KEY_ARROW_LEFT)
            {
                if(cursor_pos > 0)
                {
                    cursor_pos--;
                    printf("\x1b[D");
                    fflush(stdout);
                }
            }
            else if(key == KEY_ARROW_RIGHT)
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

    move_cursor(term_height + 1, 1);
    format_system_messages(centered, sizeof(centered), "Client disconnected. Press Ctrl + C to exit the program\n");
    printf(ANSI_RED "%s" ANSI_RESET, centered);
    close(clntSock);
}

