#include "../include/headers.h"

int handle_user_input(const char *input, int sock, size_t prefix_len)
{
    char filepath[MAX_FILENAME_LEN + 1] = {0};
    if(parse_send(input, filepath, sizeof(filepath), prefix_len) != 0)
    {
        return(-1);
    }
    return transferSocket(sock, filepath);
}

int parse_send(const char *input, char *filepath, size_t filepath_size, size_t prefix_len)
{
    const char *start = input + prefix_len;
    while(*start == ' ' || *start == '\t')
    {
        start++;
    }

    if(*start == '\0' || *start == '\n')
    {
        return(-1);
    }

    const char *end = start;
    while(*end != '\0' && *end != '\n')
    {
        end++;
    }

    while(end > start && (*(end - 1) == ' ' || *(end - 1) == '\t'))
    {
        end--;
    }

    size_t len = end - start;
    if(len >= filepath_size)
    {
        return(-1);
    }

    memcpy(filepath, start, len);
    filepath[len] = '\0';

    return (0);
}

int transferSocket(int sock, const char *filepath)
{
    FILE *file = fopen(filepath, "rb");
    if(!file)
    {
        char error_msg[BUFSIZE];
        snprintf(error_msg, sizeof(error_msg), "Could not open file: %s\n", filepath);
        add_message(error_msg, MSG_SYSTEM, NULL);
        return(-1);
    }

    if(fseek(file, 0, SEEK_END) != 0)
    {
        add_message("Failed to seek to end of file\n", MSG_SYSTEM, NULL);
        fclose(file);
        return(-1);
    }

    long file_size = ftell(file);
    if(file_size < 0)
    {
        add_message("Failed to get file size\n", MSG_SYSTEM, NULL);
        fclose(file);
        return(-1);
    }

    if(file_size > MAX_FILE_SIZE)
    {
        char error_msg[BUFSIZE];
        snprintf(error_msg, sizeof(error_msg), "File too large. Max size: %d bytes\n", MAX_FILE_SIZE);
        add_message(error_msg, MSG_SYSTEM, NULL);
        fclose(file);
        return(-1);
    }

    if(fseek(file, 0, SEEK_SET) != 0)
    {
        add_message("Failed to seek to beginning of file\n", 0, NULL);
        fclose(file);
        return(-1);
    }

    uint32_t file_size_net = htonl((unsigned int)file_size);
    if(send_all(sock, &file_size_net, sizeof(file_size_net)) < 0)
    {
        add_message("Failed to send file size\n", MSG_SYSTEM, NULL);
        fclose(file);
        return(-1);
    }

    char basename_str[MAX_FILENAME_LEN + 1];
    strncpy(basename_str, filepath, sizeof(basename_str) - 1);
    basename_str[sizeof(basename_str) - 1] = '\0';
    const char *filename = basename(basename_str);

    size_t filename_len = strlen(filename);
    if(filename_len > MAX_FILENAME_LEN)
    {
        filename_len = MAX_FILENAME_LEN;
    }

    uint8_t len_byte = (uint8_t)filename_len;
    if(send_all(sock, &len_byte, 1) < 0)
    {
        add_message("Failed to send filename length\n", MSG_SYSTEM, NULL);
        fclose(file);
        return(-1);
    }

    if(send_all(sock, filename, filename_len) < 0)
    {
        add_message("Failed to send filename\n", MSG_SYSTEM, NULL);
        fclose(file);
        return(-1);
    }

    char buffer[RECV_BUFFER];
    size_t total_sent = 0;
    for(;;)
    {
        size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
        if(bytes_read == 0)
        {
            if(ferror(file))
            {
                add_message("Error reading file\n", MSG_SYSTEM, NULL);
                fclose(file);
                return(-1);
            }
            break;
        }

        if(send_all(sock, buffer, bytes_read) < 0)
        {
            add_message("Error sending file data\n", MSG_SYSTEM, NULL);
            fclose(file);
            return(-1);
        }

        total_sent += bytes_read;
    }

    fclose(file);

    char success_msg[BUFSIZE];
    char centered[BUFSIZE];
    snprintf(success_msg, sizeof(success_msg), "File sent successfully: %zu bytes\n", total_sent);
    format_system_messages(centered, sizeof(centered), success_msg);
    add_message(centered, MSG_SYSTEM, NULL);

    return(0);
}

int rcvSocket(int sock)
{
    uint32_t file_size_net;
    ssize_t bytes_recv = recv_all(sock, &file_size_net, sizeof(file_size_net));
    if(bytes_recv != (ssize_t)sizeof(file_size_net))
    {
        add_message("Failed to receive file size\n", MSG_SYSTEM, NULL);
        return(-1);
    }

    uint32_t file_size = ntohl(file_size_net);

    if(file_size > MAX_FILE_SIZE)
    {
        char error_msg[BUFSIZE];
        snprintf(error_msg, sizeof(error_msg), "File too large. Max size: %d bytes\n", MAX_FILE_SIZE);
        add_message(error_msg, MSG_SYSTEM, NULL);
        return(-1);
    }

    uint8_t len_byte;
    bytes_recv = recv_all(sock, &len_byte, 1);
    if(bytes_recv != 1)
    {
        add_message("Failed to receive filename length\n", 0, NULL);
        return(-1);
    }

    unsigned int filename_len = (unsigned int)len_byte;
    if(filename_len == 0)
    {
        add_message("Rejected file: empty filename.\n", MSG_SYSTEM, NULL);
        return(-1);
    }

    char filename[MAX_FILENAME_LEN + 1];
    bytes_recv = recv_all(sock, filename, filename_len);
    if(bytes_recv != (ssize_t)filename_len)
    {
        add_message("Failed to receive filename\n", MSG_SYSTEM, NULL);
        return(-1);
    }
    filename[filename_len] = '\0';

    char safe_filename[MAX_FILENAME_LEN + 1];
    if(sanitize_filename(filename, safe_filename, sizeof(safe_filename)) != 0)
    {
        add_message("Rejected file: unsafe filename\n", MSG_SYSTEM, NULL);
        return(-1);
    }

    if(mkdir(DOWNLOAD_DIR, 0700) != 0 && errno != EEXIST)
    {
        add_message("Could not create download directory.\n", MSG_SYSTEM, NULL);
        return(-1);
    }

    char path[sizeof(DOWNLOAD_DIR) + 1 + MAX_FILENAME_LEN + 1];
    int plen = snprintf(path, sizeof(path), "%s/%s", DOWNLOAD_DIR, safe_filename);
    if(plen < 0 || (size_t)plen >= sizeof(path))
    {
        add_message("Download path too long\n", MSG_SYSTEM, NULL);
        return(-1);
    }

    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if(fd < 0)
    {
        char error_msg[BUFSIZE];
        snprintf(error_msg, sizeof(error_msg), "Could not create file '%s' (%s)\n", safe_filename, strerror(errno));
        add_message(error_msg, MSG_SYSTEM, NULL);
        return(-1);
    }

    FILE *file = fopen(filename, "wb");
    if(!file)
    {
        add_message("Could not open file stream\n", MSG_SYSTEM, NULL);
        close(fd);
        return(-1);
    }

    char buffer[RECV_BUFFER];
    uint32_t total_recv = 0;
    while(total_recv < file_size)
    {
        size_t remaining = file_size - total_recv;
        size_t to_recv = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
        bytes_recv = recv_all(sock, buffer, to_recv);
        if(bytes_recv <= 0)
        {
            add_message("Error receiving file data\n", MSG_SYSTEM, NULL);
            fclose(file);
            return(-1);
        }

        if(fwrite(buffer, 1, (size_t)bytes_recv, file) != (size_t)bytes_recv)
        {
            add_message("Error writing to file\n", MSG_SYSTEM, NULL);
            fclose(file);
            return(-1);
        }
        total_recv += (uint32_t)bytes_recv;
    }

    fclose(file);

    char success_msg[BUFSIZE];
    char centered[BUFSIZE];
    snprintf(success_msg, sizeof(success_msg), "File received: %s (%u bytes)\n", safe_filename, total_recv);
    format_system_messages(centered, sizeof(centered), success_msg);
    add_message(centered, MSG_SYSTEM, NULL);

    return(0);
}


ssize_t send_all(int sock, const void *buffer, size_t length)
{
    const char *buf = (const char *)buffer;
    size_t total_sent = 0;

    while(total_sent < length)
    {
        ssize_t n = send(sock, buf + total_sent, length - total_sent, 0);
        if(n < 0)
        {
            //retry on interrupted syscall
            if(errno == EINTR)
                continue;

            return(-1);
        }
        total_sent += (size_t)n;
    }

    return (ssize_t)total_sent;
}

ssize_t recv_all(int sock, void *buffer, size_t length)
{
    char *buf = (char *)buffer;
    size_t total_recv = 0;

    while(total_recv < length)
    {
        ssize_t n = recv(sock, buf + total_recv, length - total_recv, 0);
        if(n < 0)
        {
            if(errno == EINTR)
                continue;

            return(-1);
        }
        if(n == 0)
            return(ssize_t)total_recv;

        total_recv += (size_t)n;
    }

    return (ssize_t)total_recv;
}

ssize_t recv_with_timeout(int sock, void *buffer, size_t length, int timeout_sec)
{
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    int activity = select(sock + 1, &read_fds, NULL, NULL, &timeout);

    if(activity < 0)
    {
        return -1;
    }
    else if(activity == 0)
    {
        return 0;
    }

    return recv(sock, buffer, length, 0);
}
