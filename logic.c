#include "headers.h"

int handle_user_input(const char *input, int sock, size_t prefix_len)
{
    char filepath[256];

    if(!parse_send(input, filepath, sizeof(filepath), prefix_len))
    {
        format_system_messages(centered, sizeof(centered), "Failed to parse filepath.\n");
        add_message(centered, 0, NULL);
        memset(centered, 0, sizeof(centered));
        return(1);
    }

    snprintf(msg_buff, sizeof(msg_buff), "File transfer requested: %s\n", filepath);
    format_system_messages(centered, sizeof(centered), msg_buff);
    add_message(centered, 0, NULL);
    memset(centered, 0, sizeof(centered));

    int result = transferSocket(sock, filepath);
    if(result == 0)
    {
        //printf("File sent successfully!\n");
    }
    else
    {
        format_system_messages(centered, sizeof(centered), "File transfer failed.\n");
        add_message(centered, 0, NULL);
        memset(centered, 0, sizeof(centered));
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
        format_system_messages(centered, sizeof(centered), "Error: No filepath provided after ::send.\n");
        add_message(centered, 0, NULL);
        memset(centered, 0, sizeof(centered));
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
        format_system_messages(centered, sizeof(centered), msg_buff);
        add_message(centered, 0, NULL);
        memset(centered, 0, sizeof(centered));
        return(0);
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
    restore_term_state();
    move_cursor(term_height + 1, 1);
    printf("\n");
    fflush(stdout);
}

void signal_handler(int signum)
{
    if(signum == SIGINT || signum == SIGTERM)
    {
        running = 0;
        restore_term_state();
        destroy_message_buffer(&g_msg_buffer);
        fflush(stdout);
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

/*#ifdef SIGWINCH
    sa.sa_handler = sigwinch_handler;
    sigaction(SIGWINCH, &sa, NULL);
#endif

    save_terminal_state();*/
}

