#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ANSI_BG_GREEN "\e[42m"
#define ANSI_BG_CYAN "\e[46m"
#define ANSI_RESET "\e[0m"

#define BUFSIZE 3000
char servStr[3000];
static const int MAXPENDING = 5;

void HandleClient(int argc, char **argv);
void HandleServer(int argc, char *argv[]);
void HandleTCPClient(int clntSocket, char *clntName);

#endif