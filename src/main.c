#include "../include/headers.h"

volatile sig_atomic_t running = 1;
volatile sig_atomic_t g_terminal_resized = 0;
int scroll_offset = 0;

void signal_handler(int signum)
{
    if(signum == SIGINT || signum == SIGTERM)
    {
        running = 0;
        cleanup_terminal();
        exit(0);
    }
    else if(signum == SIGWINCH)
    {
        g_terminal_resized = 1;
    }
}

void setup_signal_handlers()
{
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGWINCH, &sa, NULL);
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <mode> [port] [ip/port]\n", argv[0]);
        fprintf(stderr, "Server mode: %s server <port>\n", argv[0]);
        fprintf(stderr, "Client mode: %s client <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    init_message_buffer(&g_msg_buffer);
    setup_signal_handlers();

    if(strcmp(argv[1], "server") == 0)
    {
        if(argc < 3)
        {
            fprintf(stderr, "Server mode requires a port number.\n");
            return 1;
        }
        HandleClient(argv);
    }
    else if(strcmp(argv[1], "client") == 0)
    {
        if(argc < 4)
        {
            fprintf(stderr, "Client mode requires server IP and port.\n");
            return 1;
        }
        HandleServer(argv);
    }
    else
    {
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        return 1;
    }

    cleanup_terminal();
    destroy_message_buffer(&g_msg_buffer);
    return 0;
}
