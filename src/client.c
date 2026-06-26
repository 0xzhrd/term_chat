#include "../include/headers.h"

void HandleServer(char *argv[]) 
{
    const char *ServerIP = argv[2];
    uint16_t ServerPort;

    if(parse_port(argv[3], &ServerPort) != 0)
    {
        fprintf(stderr, "Invalid port '%s' (expected 1-%d).\n", argv[3], MAX_PORT);
        exit(1);
    }

    get_terminal_size();
    save_term_state();
    clear_screen();

    char centered[BUFSIZE];
    format_system_messages(centered, sizeof(centered), "CLIENT ACTIVE\n");
    add_message(centered, 0, NULL);

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

    int rtnVal = inet_pton(AF_INET, ServerIP, &ServerAddr.sin_addr);
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

    if(connect(sock, (struct sockaddr *)&ServerAddr, sizeof(ServerAddr)) < 0)
    {
        fprintf(stderr, "connect failed.\n");
        restore_term_state();
        close(sock);
        exit(1);
    }

    char servName[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, &ServerAddr.sin_addr, servName, sizeof(servName)) == NULL)
    {
        add_message("inet_ntop() failed. Could not get server address\n", 0, NULL);
        strcpy(servName, "unknown");
    }
    const char *Name = servName;
    size_t input_len = 0;
    size_t cursor_pos = 0;
    const size_t prompt_len = 4;


    while(g_running)
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
                continue;

            add_message("select() error.\n", 0, NULL);
            break;
        }

        if(FD_ISSET(sock, &read_fds))
        {   
            char buffer[BUFSIZE];
            memset(buffer, 0, sizeof(buffer));
            ssize_t numBytes = recv(sock, buffer, (BUFSIZE - 1), 0);
            if(numBytes <= 0)
            {
                add_message("\nSERVER DISCONNECTED\n", 0, NULL);
                g_running = 0;
                break;
            } 
            buffer[numBytes] = '\0';

            if(strcmp(buffer, "FILE_RECV") == 0)
            {
                const char ack[] = "ACK";
                if(send_all(sock, ack, strlen(ack)) < 0)
                {
                    add_message("Failed to send ACK\n", MSG_SYSTEM, NULL);
                    break;
                }
                char line[BUFSIZE];

                if(rcvSocket(sock) == 0)
                {
                    format_system_messages(line, sizeof(line), "FILE RECEIVED SUCCESSFULLY.\n");
                }
                else
                {
                    format_system_messages(centered, sizeof(centered), "FILE TRANSFER FAILED.\n");
                }
            }
            else
            {
                add_message(buffer, MSG_PEER, Name);
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
                display_messages(&g_msg_buffer, Name);
                setup_input_line(input);
            }
            else if(key == KEY_PAGE_DOWN)
            {
                g_scroll_offset -= (g_term_height - 2);
                if(g_scroll_offset < 0) g_scroll_offset = 0;
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
                        g_running = 0;
                        break;
                    }
                    
                    add_message(input, MSG_SELF, Name);
                    
                    const char *command_prefix = "::send";
                    size_t prefix_len = strlen(command_prefix);

                    if(strncmp(input, command_prefix, prefix_len) == 0)
                    {
                        const char protocol[] = "FILE_RECV";
                        if(send_all(sock, protocol, strlen(protocol)) < 0)
                        {
                            add_message("send() for file transfer failed.\n", MSG_SYSTEM, NULL);
                        }
                        else
                        {
                            char ack[4] = {0};
                            ssize_t ack_received = recv_with_timeout(sock, ack, 3, ACK_TIMEOUT);
                            char line[BUFSIZE];

                            if(ack_received <= 0)
                            {
                                format_system_messages(line, sizeof(line), "Failed to receive ACK (timeout/error).\n");
                                add_message(line, MSG_SYSTEM, NULL);
                            }
                            else if(ack_received < 3 || strncmp(ack, "ACK", 3) != 0)
                            {
                                format_system_messages(line, sizeof(line), "Invalid ACK received from server.\n");
                                add_message(line, MSG_SYSTEM, NULL);
                            }
                            else if(handle_user_input(input, sock, prefix_len) != 0)
                            {
                                format_system_messages(line, sizeof(line), "File transfer encountered an error.\n");
                                add_message(line, MSG_SYSTEM, NULL);
                            }
                        }
                    }
                    else
                    {   
                        if(send_all(sock, input, strlen(input)) < 0)
                        {
                            add_message("send() failed.\n", MSG_SYSTEM, NULL);
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
                    input[cursor_pos] = (char)key;
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
    move_cursor(g_term_height + 1, 1);
    close(sock);
    printf("Disconnected from server\n");
}
