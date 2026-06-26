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

void cleanup_terminal(void)
{
    restore_term_state();
    clear_screen();
}

void format_system_messages(char *dest, size_t dest_size, const char *msg)
{
    if(dest_size == 0)
    {
        return;
    }
    
    size_t msg_len = strlen(msg);
    int padding = 0;

    if((size_t)g_term_width > msg_len)
    {
        padding = (int)(((size_t)g_term_width - msg_len) / 2U);
    }

    int written = snprintf(dest, dest_size, "%*s%s", padding, "", msg);
    if(written < 0 || (size_t)written >= dest_size)
    {
        strncpy(dest, msg, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}

void save_term_state(void)
{
    if(tcgetattr(STDIN_FILENO, &g_orig_termios) == 0)
    {
        g_term_state_saved = 1;
    }
}

void restore_term_state(void)
{
    if(g_term_state_saved)
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

void set_raw_mode(void)
{
    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int parse_port(const char *s, uint16_t *out_port)
{
    if(s == NULL || *s == '\0' || out_port == NULL)
        return(-1);

    errno = 0;
    char *end = NULL;
    long val = strtol(s, &end, 10);

    if(errno != 0 || end == s || *end != '\0')
        return(-1);

    if(val < 1 || val > MAX_PORT)
        return(-1);

    *out_port = (uint16_t)val;

    return(0);
}


void sanitize_for_display(const char *in, char *out, size_t out_size)
{
    if(out_size == 0)
        return;

    size_t j = 0;
    for(size_t i = 0; in[i] != '\0' && j + 1 < out_size; i++)
    {
        unsigned char c = (unsigned char)in[i];

        if((c >= 0x20 && c <= 0x7E) || c == '\t')
        {
            out[j++] = (char)c;
        }
        else
        {
            out[j++] = '?';
        }
    }
    out[j] = '\0';
}

int sanitize_filename(const char *in, char *out, size_t out_size)
{
    if(in == NULL || out == NULL || out_size == 0 || in[0] == '\0')
        return(-1);

    const char *base = in;
    for(const char *p = in; *p != '\0'; p++)
    {
        if(*p == '/' || *p == '\\')
            base = p + 1;
    }

    if(base[0] == '\0')
        return(-1);

    if(strcmp(base, ".") == 0 || strcmp(base, "..") == 0)
        return(-1);

    size_t j = 0;
    for(size_t i = 0; base[i] != '\0' && j + 1 < out_size; i++)
    {
        unsigned char c = (unsigned char)base[i];
        if(isalnum(c) || c == '.' || c == '_' || c == '-')
        {
            out[j++] = (char)c;
        }
        else
        {
            out[j++] = '_';
        }
    }
    out[j] = '\0';

    if(j == 0)
        return(-1);

    if(strcmp(out, ".") == 0 || strcmp(out, "..") == 0)
    {
        return(-1);
    }

    return(0);
}