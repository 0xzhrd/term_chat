#include "../include/headers.h"

void HandleClient(char *argv[])
{
    uint16_t serverPort;
    if(parse_port(argv[2], &serverPort) != 0)
    {
        fprintf(stderr, "Invalid port '%s' (expected 1-%d).\n", argv[2], MAX_PORT);
        exit(1);
    }

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

    if(listen(serverSock, MAX_PENDING) < 0)
    {
        fprintf(stderr, "listen() failed.\n");
        close(serverSock);
        exit(1);
    }

    get_terminal_size();
    save_term_state();
    clear_screen();

    char centered[BUFSIZE];
    format_system_messages(centered, sizeof(centered), "SERVER ACTIVE\n");
    add_message(centered, MSG_SYSTEM, NULL);

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
                continue;

            fprintf(stderr, "select() failed.\n");
            break;
        }

        if(activity == 0)
            continue;


        int clntSock = accept(serverSock, (struct sockaddr*) &clntAddr, &clntAddrLen);
        if(clntSock < 0)
        {
            if(errno == EINTR)
                continue;

            fprintf(stderr, "accept() failed.\n");
            continue;
        }
        
        char clntName[INET_ADDRSTRLEN];
        if(inet_ntop(AF_INET, &clntAddr.sin_addr, clntName, sizeof(clntName)) != NULL)
        {
            char line[BUFSIZE];
            char msg[BUFSIZE];
            snprintf(msg, sizeof(msg), "Handling client %s on port %d\n\n", clntName, ntohs(serverAddr.sin_port));
            format_system_messages(line, sizeof(line), msg);
            add_message(line, MSG_SYSTEM, NULL);
        }
        else
        {
            add_message("Unable to get client address\n", MSG_SYSTEM, NULL);
            strcpy(clntName, "unknown");
        }
            
        HandleTCPClient(clntSock, clntName);
    }
    close(serverSock);
}

void HandleTCPClient(int clntSock, char *clntName)
{
    char input[MAX_INPUT] = {0};
    size_t input_len = 0;
    size_t cursor_pos = 0;
    const size_t prompt_len = 4;

    set_raw_mode();
    setup_input_line(input);

    while(g_running)
    {
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
                continue;

            add_message("select() error.\n", MSG_SYSTEM, NULL);
            break;
        }

        if(FD_ISSET(clntSock, &read_fds))
        {
            char buffer[BUFSIZE];
            memset(buffer, 0, BUFSIZE);
            ssize_t numBytesRcvd = recv(clntSock, buffer, (BUFSIZE - 1), 0);
            if(numBytesRcvd <= 0)
            {
                char line[BUFSIZE];
                char msg[BUFSIZE];
                snprintf(msg, sizeof(msg), "recv() failed/ %s DISCONNECTED.\n", clntName);
                format_system_messages(line, sizeof(line), msg);            
                add_message(line, MSG_SYSTEM, NULL);
                break;
            }
            buffer[numBytesRcvd] = '\0';

            if(strcmp(buffer, "FILE_RECV") == 0)
            {
                const char ack[] = "ACK";
                if(send(clntSock, ack, strlen(ack), 0) < 0)
                {
                    add_message("Failed to send ACK\n", MSG_SYSTEM, NULL);
                    break;
                }
                char line[BUFSIZE];
                
                if(rcvSocket(clntSock) == 0)
                {
                    format_system_messages(line, sizeof(line), "FILE RECEIVED SUCCESSFULLY.\n");
                }
                else
                {
                    format_system_messages(line, sizeof(line), "FILE TRANSFER FAILED.\n");
                }
                add_message(line, MSG_SYSTEM, clntName);
            }
            else
            {
                add_message(buffer, MSG_PEER, clntName);
            }
            setup_input_line(input);
            fflush(stdout);
        }

        if(FD_ISSET(STDIN_FILENO, &read_fds))
        {
            int key = read_key();

            if(key == KEY_PAGE_UP)
            {
                g_scroll_offset += (g_term_height - 2);
                display_messages(&g_msg_buffer, clntName);
                setup_input_line(input);
            }
            else if(key == KEY_PAGE_DOWN)
            {
                g_scroll_offset -= (g_term_height - 2);
                if(g_scroll_offset < 0) g_scroll_offset = 0;
                display_messages(&g_msg_buffer, clntName);
                setup_input_line(input);
            }

            else if(key == '\n' || key == '\r')
            {
                if(input_len > 0)
                {
                    input[input_len] = '\0';

                    if(strcmp(input, "::quit") == 0)
                    {
                        add_message("\n\nDISCONNECTED.\n", MSG_SYSTEM, NULL);
                        g_running = 0;
                        break;
                    }                    
                    add_message(input, MSG_SELF, clntName);
                    
                    const char *command_prefix = "::send";
                    size_t prefix_len = strlen(command_prefix);

                    if(strncmp(input, command_prefix, prefix_len) == 0)
                    {
                        const char *filename_start = input + prefix_len;
                        if(*filename_start == '\0' || *filename_start == '\n')
                        {
                            char line[BUFSIZE];
                            format_system_messages(line, sizeof(line), "Error: No filename provided.\n");
                            add_message(line, MSG_SYSTEM, NULL);
                        }
                        else
                        {
                            const char protocol = "FILE_RECV";
                            if(send_all(clntSock, protocol, strlen(protocol)) < 0)
                                add_message("send() for file transfer failed.\n", 0, NULL);
                    
                            else
                            {
                                char ack[4] = {0};
                                ssize_t ack_received = recv_with_timeout(clntSock, ack, 3, ACK_TIMEOUT);
                                char line[BUFSIZE];

                                if(ack_received <= 0)
                                {
                                    format_system_messages(line, sizeof(line), "Failed to recveive ACK(timeout/error).\n");
                                    add_message(line, MSG_SYSTEM, NULL);
                                }
                                else if(ack_received < 3 || strncmp(ack, "ACK", 3) != 0)
                                {
                                    format_system_messages(line, sizeof(line), "Invalid ACK received from peer.\n");
                                    add_message(line, MSG_SYSTEM, NULL);
                                }
                                else if(handle_user_input(input, clntSock, prefix_len) != 0)
                                {
                                    format_system_messages(line, sizeof(line), "File transfer encountered an error.\n");
                                    add_message(line, MSG_SYSTEM, NULL);
                                }
                            }
                        }
                    }
                    else
                    {
                        if(send_all(clntSock, input, strlen(input)) < 0)
                        {
                            add_message("send() failed\n", MSG_SYSTEM, NULL);
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

    move_cursor(g_term_height + 1, 1);
    char centered[BUFSIZE];
    format_system_messages(centered, sizeof(centered), "Client disconnected. Press Ctrl + C to exit the program\n");
    printf(ANSI_RED "%s" ANSI_RESET, centered);
    fflush(stdout);
    close(clntSock);
}
