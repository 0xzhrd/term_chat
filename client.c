#include "headers.h"

void HandleServer(char *argv[]) 
{
    char clntStr[BUFSIZE];
    char *ServerIP = argv[2];
    in_port_t ServerPort = atoi(argv[3]);

    get_terminal_size();
    save_term_state();
    clear_screen();

    format_system_messages(centered, sizeof(centered), "CLIENT ACTIVE\n");
    add_message(centered, 0, NULL);
    memset(centered, 0, sizeof(centered));

    char input[MAX_INPUT] = {0};

    set_raw_mode();
    setup_input_line(input);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
    if(sock < 0)
    {
        fprintf(stderr, "socket initialization failed\n");
        restore_term_state();
        exit(1);
    }

    struct sockaddr_in ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family = AF_INET;

    int rtnVal = inet_pton(AF_INET, ServerIP, &ServerAddr.sin_addr.s_addr);
    if(rtnVal == 0)
    {
        fprintf(stderr, "inet_pton failed, invalid address string.\n");
        restore_term_state();
        close(sock);
        exit(1);
    }
    else if(rtnVal < 0)
    {
        fprintf(stderr, "inet_pton failed.\n");
        restore_term_state();
        close(sock);
        exit(1);
    }
    ServerAddr.sin_port = htons(ServerPort);

    if(connect(sock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
    {
        fprintf(stderr, "connect failed.\n");
        restore_term_state();
        close(sock);
        exit(1);
    }

    char servName[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, &ServerAddr.sin_addr.s_addr, servName, sizeof(servName)) == NULL)
    {
        add_message("inet_ntop() failed. Could not get server address\n", 0, NULL);
        strcpy(servName, "unknown");
    }
    char *Name = servName;
    size_t input_len = 0;
    size_t cursor_pos = 0;
    const size_t prompt_len = 4;


    while(running)
    {
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;

        timeout.tv_sec = 0;
        timeout.tv_usec = SELECT_TIMEOUT;

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);

        if(activity < 0)
        {
            if(errno == EINTR)
            {
                continue;
            }
            add_message("select() error.\n", 0, NULL);
            break;
        }

        if(FD_ISSET(sock, &read_fds))
        {   
            identifier = 2;
            char buffer[BUFSIZE];
            memset(buffer, 0, sizeof(buffer));
            ssize_t numBytes = recv(sock, buffer, (BUFSIZE - 1), 0);

            if(numBytes <= 0)
            {
                add_message("\nSERVER DISCONNECTED\n", 0, NULL);

                restore_term_state();
                close(sock);
                exit(1);
            } 


            buffer[numBytes] = '\0';
            if(strcmp(buffer, "FILE_RECV") == 0)
            {
                const char ack[] = "ACK";
                if(send(sock, ack, strlen(ack), 0) < 0)
                {
                    add_message("Failed to send ACK\n", 0, NULL);
                    break;
                }

                if(rcvSocket(sock) == 0)
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

                        clear_screen();
                        add_message("\n\nDISCONNECTED.\n", 0, NULL);
                        memset(input, 0, sizeof(input));
                        restore_term_state();
                        running = 0;
                        exit(0);
                    }
                    
                    identifier = 1;
                    add_message(input, identifier, Name);
                    
                    strncpy(clntStr, input, sizeof(clntStr) - 1);
                    clntStr[sizeof(clntStr) - 1] = '\0';
                    
                    const char *command_prefix = "::send";
                    size_t prefix_len = strlen(command_prefix);

                    if(strncmp(clntStr, command_prefix, prefix_len) == 0)
                    {
                        const char protocol[] = "FILE_RECV";
                        size_t protocolLen = strlen(protocol);
                        ssize_t bytesSent = send(sock, protocol, protocolLen, 0);
                        
                        if(bytesSent < 0 || (size_t)bytesSent != protocolLen)
                        {
                            add_message("send() for file transfer failed.\n", 0, NULL);
                        }
                        else
                        {
                            char ack[4] = {0};
                            ssize_t ack_received = recv_with_timeout(sock, ack, 3, ACK_TIMEOUT);
                            
                            if(ack_received <= 0)
                            {
                                format_system_messages(centered, sizeof(centered), "Failed to receive ACK (timeout/error).\n");
                                add_message(centered, 0, NULL);
                                memset(centered, 0, sizeof(centered));
                            }
                            else if(strncmp(ack, "ACK", 3) != 0)
                            {
                                format_system_messages(centered, sizeof(centered), "Invalid ACK received from server.\n");
                                add_message(centered, 0, NULL);
                                memset(centered, 0, sizeof(centered));
                            }
                            else if(handle_user_input(clntStr, sock, prefix_len) != 0)
                            {
                                format_system_messages(centered, sizeof(centered), "File transfer encountered an error.\n");
                                add_message(centered, 0, NULL);
                                memset(centered, 0, sizeof(centered));
                            }
                        }
                    }
                    else
                    {
                        size_t clntStrLen = strlen(clntStr);
                        ssize_t numBytesSent = send(sock, clntStr, clntStrLen, 0);
                        
                        if(numBytesSent < 0)
                        {
                            add_message("send() failed.\n", 0, NULL);
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

                    printf("\x1b[%zuG", (size_t)(prompt_len) + cursor_pos + 1);
                    fflush(stdout);
                }
            }
        }
    }

    restore_term_state();
    move_cursor(term_height + 1, 1);
    close(sock);
    printf("Disconnected from server\n");
}
