#include "headers.h"

int handle_user_input(const char *input, int sock, size_t prefix_len)
{
    char filepath[256];

    if(!parse_send(input, filepath, sizeof(filepath), prefix_len))
    {
        add_message("Failed to parse filepath\n", 0, NULL);
        return(1);
    }

    snprintf(msg_buff, sizeof(msg_buff), "\t\t\tFile transfer requested: %s\n", filepath);
    add_message(msg_buff, 0, NULL);

    int result = transferSocket(sock, filepath);
    if(result == 0)
    {
        //printf("File sent successfully!\n");
    }
    else
    {
        add_message("File transfer failed.\n", 0, NULL);
    }

    return(result);
}


int parse_send(const char *input, char *filepath, size_t filepath_size, size_t prefix_len)
{
    if(input == NULL || filepath == NULL || filepath_size == 0)
    {
        return(0);
    }

    const char *ptr = input + prefix_len;

    while(*ptr != '\0' && isspace((unsigned char)*ptr))
    {
        ptr++;
    }

    if(*ptr == '\0')
    {
        add_message("Error: No filepath provided after ::send\n", 0, NULL);
        return(0);
    }

    size_t path_len = strlen(ptr);
    while(path_len > 0 && isspace((unsigned char)ptr[path_len - 1]))
    {
        path_len--;
    }

    if(path_len >= filepath_size)
    {
        snprintf(msg_buff, sizeof(msg_buff), "Error: filepath too long (%zu characters, max %zu)\n", path_len, filepath_size - 1);
        add_message(msg_buff, 0, NULL);
    }

    strncpy(filepath, ptr, path_len);
    filepath[path_len] = '\0';
    return(1);
}






void init_message_buffer(MessageBuffer *mb)
{
    memset(mb->messages, 0, sizeof(mb->messages));
    memset(mb->message_types, 0, sizeof(mb->message_types));
    mb->msg_count = 0;
    pthread_mutex_init(&mb->lock, NULL);
}

void destroy_message_buffer(MessageBuffer *mb)
{
    pthread_mutex_destroy(&mb->lock);
}

void cleanup_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    move_cursor(term_height + 1, 1);
    printf("\n");
    fflush(stdout);
}

void signal_handler(int signum)
{
    if(signum == SIGINT || signum == SIGTERM)
    {
        g_running = 0;
        cleanup_terminal();
        destroy_message_buffer(&g_msg_buffer);
        exit(0);
    }
}

void setup_signal_handlers()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    tcgetattr(STDIN_FILENO, &orig_termios);
}