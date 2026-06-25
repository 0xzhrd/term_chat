/* headers.h      : defines objects, function definitions, and all shared declarations for term_chat
   author         : osas
   date           : last year ish
   revion history : fixing a lot of misconfigurations as wekk as security vulnerabilities that stemmed 
                    from inadequate knowledge and a lack of adeptness with C 
                    revised on 25/6/26. 
 */

#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
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


#define ANSI_BG_GREEN "\x1b[0;92m"
#define ANSI_BG_CYAN "\x1b[0;96m"
#define ANSI_RED "\x1b[1;91m"
#define ANSI_BG_YELLOW "\x1b[1;33m" 
#define ANSI_RESET "\x1b[0m"

//transfer limits, sizing and timeouts
#define OUTPUT_BUFFER_SIZE (80 * 40 * 20)
#define BUFSIZE             3000
#define MAX_MESSAGES        50
#define MAX_INPUT           250
#define MAX_FILE_SIZE      (50 * 1024 * 1024) //50MB
#define SELECT_TIMEOUT      50000
#define RECV_BUFFER         8192
#define ACK_TIMEOUT         5
#define MAX_PENDING         5
#define MAX_FILENAME_LEN    255
#define MAX_PORT            65535
#define DOWNLOAD_DIR        "downloads" 


#define KEY_ARROW_RIGHT     1002
#define KEY_ARROW_LEFT      1003
#define KEY_PAGE_UP         1004
#define KEY_PAGE_DOWN       1005

#define MSG_SYSTEM          0
#define MSG_SELF            1
#define MSG_PEER            2


typedef struct 
{
    char messages[MAX_MESSAGES][MAX_INPUT];
    int  message_types[MAX_MESSAGES];
    int  msg_count;
    pthread_mutex_t lock;
} MessageBuffer;


extern MessageBuffer g_msg_buffer;
extern struct termios orig_termios;
extern int g_term_height;
extern int g_term_width;
extern int g_scroll_offset;
extern volatile sig_atomic_t g_running;
extern volatile sig_atomic_t g_terminal_resized;
extern volatile sig_atomic_t g_term_state_saved;


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

void get_terminal_size(void);
void clear_screen(void);
void move_cursor(int row, int col);
void clear_line(void);
int read_key(void);
void display_messages(MessageBuffer *mb, char *clientName);
void add_message(const char *text, int identifier, char *Name);
void setup_input_line(const char *current_input);

void init_message_buffer(MessageBuffer *mb);
void destroy_message_buffer(MessageBuffer *mb);
void signal_handler(int signum);
void cleanup_terminal(void);
void setup_signal_handlers(void);
void format_system_messages(char *dest, size_t dest_size, const char *msg);

void save_term_state(void);
void restore_term_state(void);
void set_raw_mode(void);

int parse_port(const char *s, uint16_t *out_port);
void sanitize_for_display(const char *in, char *out, size_t out_size);
int sanitize_filename(const char *in, char *out, size_t out_size);

#endif
