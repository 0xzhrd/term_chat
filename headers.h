#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <signal.h>
#include <termios.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>

#define ANSI_BG_GREEN "\e[0;92m"
#define ANSI_BG_CYAN "\e[0;96m"
#define ANSI_BG_YELLOW "\e[1;33m" 
#define ANSI_RESET "\e[0m"



#define OUTPUT_BUFFER_SIZE (80 * 40 * 20)
#define BUFSIZE 3000
#define MAX_MESSAGES 50
#define MAX_INPUT 250
#define MAX_FILE_SIZE (50 * 1024 * 1024)
#define SELECT_TIMEOUT 50000
#define RECV_BUFFER 8192
#define ACK_TIMEOUT 5
#define MAX_FILENAME_LEN 255
#define KEY_ARROW_RIGHT 1002
#define KEY_ARROW_LEFT  1003

typedef struct 
{
    char messages[MAX_MESSAGES][MAX_INPUT];
    int message_types[MAX_MESSAGES];
    int msg_count;
    pthread_mutex_t lock;
} MessageBuffer;


char msg_buff[BUFSIZE];
MessageBuffer g_msg_buffer;
static struct termios orig_termios;
static int chat_width, chat_height;
static const int MAXPENDING = 5;
int term_height, term_width;
int identifier;
int message_types[MAX_MESSAGES];
extern volatile sig_atomic_t g_running;


void HandleClient(char **argv);
void HandleServer(char *argv[]);
void HandleTCPClient(int clntSock, char *clntName);
int transferSocket(int sock, const char *filepath);
int rcvSocket(int sock);
int handle_user_input(const char *input, int sock, size_t prefix_len);
int parse_send(const char *input, char *filepath, size_t filepath_size, size_t prefix_len);

ssize_t send_all(int sock, const void *buffer, size_t length);
ssize_t recv_all(int sock, void *buffer, size_t length);
ssize_t recv_with_timeout(int sock, void *buffer, size_t length, int timeout_sec);

void get_terminal_size();
void clear_screen();
void move_cursor(int row, int col);
void clear_line();
int read_key();
void display_messages(MessageBuffer *mb, char *clientName);
void add_message(const char *text, int identifier, char *Name);
void setup_input_line(const char *current_input);

void init_message_buffer(MessageBuffer *mb);
void destroy_message_buffer(MessageBuffer *mb);
void signal_handler(int signum);
void cleanup_terminal();
void setup_signal_handlers();


#endif