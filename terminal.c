#include "headers.h"

void get_terminal_size()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    term_height = ws.ws_row;
    term_width = ws.ws_col;
}

void clear_screen()
{
    printf("\033[2J");
    fflush(stdout);
}

void move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row, col);
    fflush(stdout);
}

void clear_line()
{
    printf("\033[K");
    fflush(stdout);
}

void display_messages(MessageBuffer *mb, char *clientName)
{
    pthread_mutex_lock(&mb->lock);
    printf("\033[s");
    move_cursor(1, 1);

    int display_count = (mb->msg_count < term_height - 2) ? mb->msg_count : term_height - 2;
    int start_idx = (mb->msg_count < term_height - 2) ? 0 : mb->msg_count - (term_height - 2);

    for(int i = 0; i < display_count; i++)
    {
        move_cursor(i + 1, 1);
        clear_line();

        int msg_idx = start_idx + i;
        int type = mb->message_types[msg_idx % MAX_MESSAGES];

        if(type == 0)
        {
            printf(ANSI_BG_YELLOW);
            printf("%s\n", mb->messages[msg_idx % MAX_MESSAGES]);
            printf(ANSI_RESET);
        }
        else if(type == 1)
        {
            printf(ANSI_BG_GREEN);
            printf("\r[me]: %s\n", mb->messages[msg_idx % MAX_MESSAGES]);
            printf(ANSI_RESET);
        }
        else if(type == 2)
        {
            printf(ANSI_BG_CYAN);
            printf("[%s]: %s\n", clientName, mb->messages[msg_idx % MAX_MESSAGES]);
            printf(ANSI_RESET);
        }
    }

    printf("\033[u");
    fflush(stdout);

    pthread_mutex_unlock(&mb->lock);
}

void add_message(const char *text, int identifier, char *Name)
{
    char *clientName = Name;

    pthread_mutex_lock(&g_msg_buffer.lock);

    int idx = g_msg_buffer.msg_count % MAX_MESSAGES;
    strncpy(g_msg_buffer.messages[idx], text, MAX_INPUT - 1);
    g_msg_buffer.messages[idx][MAX_INPUT - 1] = '\0';
    g_msg_buffer.message_types[idx] = identifier;
    g_msg_buffer.msg_count++;

    pthread_mutex_unlock(&g_msg_buffer.lock);
    display_messages(&g_msg_buffer, clientName);
}

void setup_input_line(const char *current_input)
{
    move_cursor(term_height, 1);
    clear_line();
    printf(">_: %s", current_input);
    fflush(stdout);
}



int read_key()
{
    char seq[3];
    
    if(read(STDIN_FILENO, &seq[0], 1) != 1)
        return -1;

    if(seq[0] != '\x1b')
        return seq[0];

    if(read(STDIN_FILENO, &seq[1], 1) != 1)
        return '\x1b';  

    if(seq[1] == '[')
    {
        if(read(STDIN_FILENO, &seq[2], 1) != 1)
            return '\x1b';
        
        switch(seq[2])
        {
            case 'C': return KEY_ARROW_RIGHT;
            case 'D': return KEY_ARROW_LEFT;
        }
    }
    
    return seq[0];
}
