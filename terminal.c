#include "headers.h"

int scroll_offset = 0;

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
    char local_messages[MAX_MESSAGES][MAX_INPUT];
    int local_types[MAX_MESSAGES];
    int local_count;

    pthread_mutex_lock(&mb->lock);
    memcpy(local_messages, mb->messages, sizeof(local_messages));
    memcpy(local_types, mb->message_types, sizeof(local_types));
    local_count = mb->msg_count;
    pthread_mutex_unlock(&mb->lock);

    printf("\033[s");
    move_cursor(1, 1);
    
    int max_displayable = term_height - 2;
    int total_messages = local_count;

    int max_scroll = (total_messages > max_displayable) ? total_messages - max_displayable : 0;
    if(scroll_offset < 0) 
    {
        scroll_offset = 0;
    }
    if(scroll_offset > max_scroll)
    {
        scroll_offset = max_scroll;
    }

    int display_count = (total_messages < max_displayable) ? total_messages : max_displayable;
    int start_idx = max_scroll - scroll_offset;

    if(scroll_offset > 0)
    {
        move_cursor(1, term_width - 20);
        printf(ANSI_BG_YELLOW "[scrolled: -%d]" ANSI_RESET, scroll_offset);
    }

    for(int i = 0; i < display_count; i++)
    {
        move_cursor(i + 1, 1);
        clear_line();
        int msg_idx = start_idx + i;
        int type = local_types[msg_idx % MAX_MESSAGES];

        if(type == 0)
        {
            printf(ANSI_BG_YELLOW);
            printf("%s\n", local_messages[msg_idx % MAX_MESSAGES]);
            printf(ANSI_RESET);
        }
        else if(type == 1)
        {
            printf(ANSI_BG_GREEN);
            printf("\r[me]: %s\n", local_messages[msg_idx % MAX_MESSAGES]);
            printf(ANSI_RESET);
        }
        else if(type == 2)
        {
            printf(ANSI_BG_CYAN);
            printf("[%s]: %s\n", clientName, local_messages[msg_idx % MAX_MESSAGES]);
            printf(ANSI_RESET);
        }
    }

    printf("\033[u");
    fflush(stdout);
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

void save_term_state()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
}

void restore_term_state()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    printf("\033[?25h");
    printf("\033[0m");
    fflush(stdout);
}

void set_raw_mode()
{
    struct termios new_tio;
    new_tio = orig_termios;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
}


int read_key()
{
    char seq[6];
    
    if(read(STDIN_FILENO, &seq[0], 1) != 1)
    {
        return -1;
    }

    if(seq[0] != '\x1b')
    {
        return seq[0];
    }

    if(read(STDIN_FILENO, &seq[1], 1) != 1)
    {
        return '\x1b';  
    }

    if(seq[1] == '[')
    {
        if(read(STDIN_FILENO, &seq[2], 1) != 1)
        {
            return '\x1b';
        }

        if(seq[2] >= '0' && seq[2] <= '9')
        {
            if(read(STDIN_FILENO, &seq[3], 1) != 1)
            {
                return '\x1b';
            }
            if(seq[3] == '~')
            {
                if(seq[2] == '5') return KEY_PAGE_UP;
                if(seq[2] == '6') return KEY_PAGE_DOWN;
            }
        }
        
        switch(seq[2])
        {
            case 'C': return KEY_ARROW_RIGHT;
            case 'D': return KEY_ARROW_LEFT;
        }
    } 
    return seq[0];
}

void format_system_messages(char *dest, size_t dest_size, const char *msg)
{
    int msg_len = strlen(msg);
    int padding = (term_width > msg_len) ? (term_width - msg_len) / 2 : 0;

    if(padding > 0 && padding < (int)dest_size - msg_len - 1)
    {
        memset(dest, ' ', padding);
        strncpy(dest + padding, msg, dest_size - padding - 1);
        dest[dest_size - 1] = '\0';
    }
    else
    {
        strncpy(dest, msg, dest_size - 1);
        dest[dest_size - 1] = '\0';
    }
}
