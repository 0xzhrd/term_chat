/* main.c           : 
   author           :
   date             :
   revision history :
*/

#include "../include/headers.h"

MessageBuffer  g_msg_buffer;
struct termios g_orig_termios;
int g_term_height = 24;
int g_term_width = 80;
volatile sig_atomic_t g_running = 1;
volatile sig_atomic_t g_terminal_resized = 0;
volatile sig_atomic_t g_term_state_saved = 0;
int g_scroll_offset = 0;


void signal_handler(int signum)
{
    if(signum == SIGINT || signum == SIGTERM)
    {
        g_running = 0;
        if(g_term_state_saved)
        {
            (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        }
    }
    else if(signum == SIGWINCH)
    {
        g_terminal_resized = 1;
    }
}

void setup_signal_handlers()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    (void)sigaction(SIGINT, &sa, NULL);
    (void)sigaction(SIGTERM, &sa, NULL);
    (void)sigaction(SIGWINCH, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <mode> [port] [ip/port]\n", argv[0]);
        fprintf(stderr, "Server mode: %s server <port>\n", argv[0]);
        fprintf(stderr, "Client mode: %s client <server_ip> <server_port>\n", argv[0]);
        return(1);
    }

    init_message_buffer(&g_msg_buffer);
    setup_signal_handlers();

    if(strcmp(argv[1], "server") == 0)
    {
        if(argc < 3)
        {
            fprintf(stderr, "Server mode requires a port number.\n");
            destroy_message_buffer(&g_msg_buffer);
            return(1);
        }
        HandleClient(argv);
    }
    else if(strcmp(argv[1], "client") == 0)
    {
        if(argc < 4)
        {
            fprintf(stderr, "Client mode requires server IP and port.\n");
            return(1);
        }
        HandleServer(argv);
    }
    else
    {
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        destroy_message_buffer(&g_msg_buffer);
        return(1);
    }

    cleanup_terminal();
    destroy_message_buffer(&g_msg_buffer);
    return(0);
}
