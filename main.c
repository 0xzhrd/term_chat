#include "headers.h"

int main(int argc, char *argv[])
{
    if(argc < 3 || argc > 4)
    {
        printf("\n\nUSAGE: \n");
        printf("To use as a server: <program name> --server <receiving port>\n");
        printf("To use as a client: <program name> --client <server IP> <server port>\n");
        return(1);
    }
    if(argc == 3 && (strcmp(argv[1],"--server") != 0))
    {
        printf("Incorrect usage!!");
        printf("To use as a server:\n\n\t\t\t <program name> --server <receiving port>\n");
        return(1);
    }
    if(argc == 4 && (strcmp(argv[1],"--client") != 0))
    {
        printf("Incorrect usage!!");
        printf("To use as a client:\n\n\t\t\t <program name> --client <server IP> <server port>\n");
        return(1);
    }

    if(strcmp(argv[1],"--server") == 0)
    {
        HandleClient(argc, argv);
    }

    if(strcmp(argv[1],"--client") == 0)
    {
        HandleServer(argc, argv);
    }

}

