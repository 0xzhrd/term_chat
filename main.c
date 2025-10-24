#include "headers.h"

volatile sig_atomic_t running = 1;
volatile sig_atomic_t g_terminal_resized = 0;


int main(int argc, char *argv[])
{    
    init_message_buffer(&g_msg_buffer);
    setup_signal_handlers();

    if(argc < 3 || argc > 4)
    {
        printf("\n\nUSAGE: \n");
        printf("To use as a server: <program name> --server <receiving port>\n");
        printf("To use as a client: <program name> --client <server IP> <server port>\n");
        destroy_message_buffer(&g_msg_buffer);
        return(1);
    }
    if(argc == 3 && (strcmp(argv[1],"--server") != 0))
    {
        printf("Incorrect usage!!");
        printf("To use as a server:\n\n\t\t\t <program name> --server <receiving port>\n");
        destroy_message_buffer(&g_msg_buffer);
        return(1);
    }
    if(argc == 4 && (strcmp(argv[1],"--client") != 0))
    {
        printf("Incorrect usage!!");
        printf("To use as a client:\n\n\t\t\t <program name> --client <server IP> <server port>\n");
        destroy_message_buffer(&g_msg_buffer);
        return(1);
    }
    
    if(strcmp(argv[1],"--server") == 0)
    {
        HandleClient(argv);
    }

    else if(strcmp(argv[1],"--client") == 0)
    {
        HandleServer(argv);
    }

    cleanup_terminal();
    destroy_message_buffer(&g_msg_buffer);
    return(0);
}
