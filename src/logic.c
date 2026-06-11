#include "../include/headers.h"

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
    clear_screen();
}

void format_system_messages(char *dest, size_t dest_size, const char *msg)
{
    int padding = (term_width - strlen(msg)) / 2;
    if(padding < 0) padding = 0;

    int written = snprintf(dest, dest_size, "%*s%s", padding, "", msg);
    if(written < 0 || written >= (int)dest_size)
    {
        strncpy(dest, msg, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

void save_term_state()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
}

void restore_term_state()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void set_raw_mode()
{
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
