#include "../include/headers.h"

void get_terminal_size()
{
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        term_width = 80;
        term_height = 24;
    }
    else
    {
        term_width = ws.ws_col;
        term_height = ws.ws_row;
    }
}

void clear_screen()
{
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

void move_cursor(int row, int col)
{
    printf("\x1b[%d;%dH", row, col);
    fflush(stdout);
}

void clear_line()
{
    printf("\x1b[2K");
    fflush(stdout);
}

int read_key()
{
    unsigned char seq[6];
    if(read(STDIN_FILENO, &seq[0], 1) != 1) return -1;
    if(seq[0] != '\x1b')
    {
        return seq[0];
    }

    if(read(STDIN_FILENO, &seq[1], 1) != 1) return -1;
    if(seq[1] != '[')
    {
        if(seq[1] == 'O')
        {
            if(read(STDIN_FILENO, &seq[2], 1) != 1) return -1;
            if(seq[2] == 'A' || seq[2] == 'B' || seq[2] == 'C' || seq[2] == 'D')
            {
                return KEY_ARROW_RIGHT + (seq[2] - 'C');
            }
        }
        return -1;
    }

    if(read(STDIN_FILENO, &seq[2], 1) != 1) return -1;

    if(seq[2] >= '0' && seq[2] <= '9')
    {
        if(read(STDIN_FILENO, &seq[3], 1) != 1) return -1;
        if(seq[3] == '~')
        {
            switch(seq[2])
            {
                case '1':
                case '7':
                    return KEY_PAGE_UP;
                case '4':
                case '8':
                    return KEY_PAGE_DOWN;
            }
        }
        return -1;
    }
    else
    {
        switch(seq[2])
        {
            case 'A':
                return KEY_PAGE_UP;
            case 'B':
                return KEY_PAGE_DOWN;
            case 'C':
                return KEY_ARROW_RIGHT;
            case 'D':
                return KEY_ARROW_LEFT;
        }
    }
    return -1;
}

void display_messages(MessageBuffer *mb, char *clientName)
{
    pthread_mutex_lock(&mb->lock);
    clear_screen();

    int start_index = mb->msg_count - (term_height - 2) + scroll_offset;
    if(start_index < 0) start_index = 0;
    int end_index = mb->msg_count + scroll_offset;
    if(end_index > mb->msg_count) end_index = mb->msg_count;

    for(int i = start_index; i < end_index; i++)
    {
        if(mb->message_types[i] == 1)
        {
            printf(ANSI_BG_CYAN "You:" ANSI_RESET " %s\n", mb->messages[i]);
        }
        else if(mb->message_types[i] == 2)
        {
            printf(ANSI_BG_GREEN "%s:" ANSI_RESET " %s\n", clientName, mb->messages[i]);
        }
        else
        {
            printf("%s\n", mb->messages[i]);
        }
    }

    pthread_mutex_unlock(&mb->lock);
}

void add_message(const char *text, int identifier, char *Name)
{
    pthread_mutex_lock(&g_msg_buffer.lock);

    if(g_msg_buffer.msg_count >= MAX_MESSAGES)
    {
        memmove(g_msg_buffer.messages[0], g_msg_buffer.messages[1], 
                sizeof(g_msg_buffer.messages[0]) * (MAX_MESSAGES - 1));
        memmove(&g_msg_buffer.message_types[0], &g_msg_buffer.message_types[1],
                sizeof(int) * (MAX_MESSAGES - 1));
        g_msg_buffer.msg_count = MAX_MESSAGES - 1;
    }

    strncpy(g_msg_buffer.messages[g_msg_buffer.msg_count], text, MAX_INPUT - 1);
    g_msg_buffer.messages[g_msg_buffer.msg_count][MAX_INPUT - 1] = '\0';
    g_msg_buffer.message_types[g_msg_buffer.msg_count] = identifier;
    g_msg_buffer.msg_count++;

    display_messages(&g_msg_buffer, Name);

    pthread_mutex_unlock(&g_msg_buffer.lock);
}

void setup_input_line(const char *current_input)
{
    move_cursor(term_height, 1);
    clear_line();
    printf("You: %s", current_input);
    fflush(stdout);
}
