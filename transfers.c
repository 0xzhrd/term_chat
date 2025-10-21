#include "headers.h"

int rcvSocket(int sock)
{
    char buffer[RECV_BUFFER];

    uint32_t file_size_net;
    if(recv_all(sock, &file_size_net, sizeof(file_size_net)) < 0)
    {
        add_message("Failed to receive file size\n", 0, NULL);
        return(1);
    }

    uint32_t file_size = ntohl(file_size_net);
    if(file_size == 0)
    {
        add_message("\t\t\t\tInvalid file size: 0 bytes\n", 0, NULL);
        return(1);
    }

    snprintf(msg_buff, sizeof(msg_buff), "\t\t\tReceiving file of size: %u bytes\n", file_size);
    add_message(msg_buff, 0, NULL);


    if(file_size > MAX_FILE_SIZE)
    {
        snprintf(msg_buff, sizeof(msg_buff), "\t\t\tFile size too large: %u bytes (max 50MB)\n", file_size);
        add_message(msg_buff, 0, NULL);
        return(1);
    }

    uint32_t filename_len_net;
    if(recv_all(sock, &filename_len_net, sizeof(filename_len_net)) < 0)
    {
        add_message("Failed to receive filename length\n", 0, NULL);
        return(1);
    }

    uint32_t filename_len = ntohl(filename_len_net);

    if(filename_len == 0 || filename_len >= MAX_FILENAME_LEN)
    {
        snprintf(msg_buff, sizeof(msg_buff), "Invalid filename length: %u\n", filename_len);
        add_message(msg_buff, 0, NULL);
        return(1);
    }

    char filename[MAX_FILENAME_LEN + 1];
    memset(filename, 0, sizeof(filename));

    if(recv_all(sock, filename, filename_len) < 0)
    {
        add_message("Failed to receive filename\n", 0, NULL);
        return(1);
    }
    filename[filename_len] = '\0';

    snprintf(msg_buff, sizeof(msg_buff), "\t\t\t\tReceiving file: %s\n", filename);
    add_message(msg_buff, 0, NULL);
    
    FILE *recv_file = fopen(filename, "wb");
    if(!recv_file)
    {
        snprintf(msg_buff, sizeof(msg_buff), "Failed to create file: %s\n", filename);
        add_message(msg_buff, 0, NULL);
        perror("fopen");
        return(1);
    }

    size_t total_received = 0;
    add_message("\t\t\t\tReceiving data...\n", 0, NULL);

    while(total_received < file_size)
    {
        size_t to_receive = file_size - total_received;
        if(to_receive > sizeof(buffer))
        {
            to_receive = sizeof(buffer);
        }

        ssize_t received = recv(sock, buffer, to_receive, 0);

        if(received < 0)
        {
            add_message("recv() failed\n", 0, NULL);
            perror("recv");
            fclose(recv_file);
            return(1);
        }
        else if(received == 0)
        {
            snprintf(msg_buff, sizeof(msg_buff), "Connection closed prematurely. Expected %u bytes, got %zu bytes\n", file_size, total_received);
            add_message(msg_buff, 0, NULL);
            fclose(recv_file);
            return(1);
        }

        size_t written = fwrite(buffer, 1, received, recv_file);
        if(written != (size_t)received)
        {
            snprintf(msg_buff, sizeof(msg_buff), "\t\t\tfwrite() failed: wrote %zu bytes, expected %zd bytes\n", written, received);
            add_message(msg_buff, 0, NULL);
            perror("fwrite");
            fclose(recv_file);
            return(1);
        }

        total_received += received;

        static size_t last_progress = 0;

        if(total_received - last_progress > file_size / 20 || total_received - last_progress > 1024 * 1024
            || total_received == file_size)
        {
            float progress = (float)total_received / file_size * 100.0f;
            snprintf(msg_buff, sizeof(msg_buff), "\t\t\tProgress: %.1f%% (%.2f/%.2f MB)", progress, total_received / (1024.0 * 1024.0), file_size / (1024.0 * 1024.0));
            add_message(msg_buff, 0, NULL);
            last_progress = total_received;
        }
    }

    fflush(recv_file);
    snprintf(msg_buff, sizeof(msg_buff), "\n\t\t\tFile received successfully: %zu bytes\n", total_received);
    add_message(msg_buff, 0, NULL);
    fclose(recv_file);

    return(0);
}

ssize_t recv_all(int sock, void *buffer, size_t length)
{
    size_t total_received = 0;
    char *ptr = (char *)buffer;

    while(total_received < length)
    {
        ssize_t received = recv(sock, ptr + total_received, length - total_received, 0);
        if(received < 0)
        {
            return(-1);
        }
        else if(received == 0)
        {
            return(-1);
        }
        total_received += received;
    }
    return(total_received);
}

ssize_t recv_with_timeout(int sock, void *buffer, size_t length, int timeout_sec)
{
    fd_set read_fds;
    struct timeval timeout;

    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);

    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;

    int result = select(sock + 1, &read_fds, NULL, NULL, &timeout);

    if(result < 0)
    {
        return(-1);
    }
    else if(result == 0)
    {
        errno = ETIMEDOUT;
        return(-1);
    }

    return(recv(sock, buffer, length, 0));
}


int transferSocket(int sock, const char *filepath)
{
    char filename[MAX_FILENAME_LEN + 1];
    char buffer[RECV_BUFFER];

    const char *filename_start = strrchr(filepath, '/');
    if(filename_start == NULL)
    {
        filename_start = filepath;//no path separators use whole string as filename
    }
    else
    {
        filename_start++;
    }

    strncpy(filename, filename_start, sizeof(filename) - 1);
    filename[sizeof(filename) - 1] = '\0';

    size_t fname_len = strlen(filename);
    if(fname_len == 0 || fname_len >= MAX_FILENAME_LEN)
    {
        snprintf(msg_buff, sizeof(msg_buff), "Invalid filename length: %zu\n", fname_len);
        add_message(msg_buff, 0, NULL);
        return(1);
    }

    FILE *send_file = fopen(filepath, "rb");
    if(!send_file)
    {
        snprintf(msg_buff, sizeof(msg_buff), "Failed to open file: %s\n", filepath);
        add_message(msg_buff, 0, NULL);
        perror("fopen");
        return(1);
    }

    if(fseek(send_file, 0, SEEK_END) != 0)
    {
        add_message("Failed to seek to the end of file\n", 0, NULL);
        perror("fseek");
        fclose(send_file);
        return(1);
    }

    long file_size = ftell(send_file);
    if(file_size < 0)
    {
        add_message("Failed to get file size\n", 0, NULL);
        perror("ftell");
        fclose(send_file);
        return(1);
    }

    if(fseek(send_file, 0, SEEK_SET) != 0)
    {
        add_message("Failed to seek to the beginning of file\n", 0, NULL);
        perror("fseek");
        fclose(send_file);
        return(1);
    }

    snprintf(msg_buff, sizeof(msg_buff), "\t\t\t\tFile size: %ld bytes\n", file_size);
    add_message(msg_buff, 0, NULL);

    if(file_size == 0)
    {
        add_message("Cannot send empty file\n", 0, NULL);
        fclose(send_file);
        return(1);
    }
    if(file_size > MAX_FILE_SIZE)
    {
        snprintf(msg_buff, sizeof(msg_buff), "File too large: %ld bytes (max %d)\n", file_size, MAX_FILE_SIZE);
        add_message(msg_buff, 0, NULL);
        fclose(send_file);
        return(1);
    }

    uint32_t file_size_net = htonl((uint32_t)file_size);

    if(send_all(sock, &file_size_net, sizeof(file_size_net)) < 0)
    {
        add_message("Failed to send file size\n", 0, NULL);
        perror("send");
        fclose(send_file);
        return(1);
    }

    uint32_t filename_len = (uint32_t)fname_len;
    uint32_t filename_len_net = htonl(filename_len);
    if(send_all(sock, &filename_len_net, sizeof(filename_len_net)) < 0)
    {
        add_message("Failed to send filename length", 0, NULL);
        perror("send");
        fclose(send_file);
        return(1);
    }

    if(send_all(sock, filename, filename_len) < 0 )
    {
        add_message("Failed to send filename\n", 0, NULL);
        perror("send");
        fclose(send_file);
        return(1);
    } 

    size_t total_sent = 0;
    add_message("\t\t\t\tsending file...\n", 0, NULL);

    while(total_sent < (size_t)file_size)
    {
        size_t chunk_size = fread(buffer, 1, sizeof(buffer), send_file);

        if(chunk_size == 0)
        {
            if(feof(send_file))
            {
                break;
            }
            else
            {
                add_message("fread() error", 0, NULL);
                perror("fread");
                fclose(send_file);
                return(1);
            }
        }

        if(send_all(sock, buffer, chunk_size) < 0)
        {
            add_message("File transfer failed\n", 0, NULL);
            perror("send");
            fclose(send_file);
            return(1);
        }

        total_sent += chunk_size;

        static size_t last_progress = 0;
        float progress = (float)total_sent / file_size * 100.0f;
        if(total_sent - last_progress > (size_t)file_size / 10 || total_sent == (size_t)file_size)
        {
            snprintf(msg_buff, sizeof(msg_buff), "\r\t\t\tProgress: %.2f%% (%zu/%ld bytes)", progress, total_sent, file_size);
            add_message(msg_buff, 0, NULL);
            last_progress = total_sent;
            fflush(stdout);
        }
    }

    fclose(send_file);
    return(0);
}

ssize_t send_all(int sock, const void *buffer, size_t length)
{
    size_t total_sent = 0;
    
    const char *ptr = (const char *)buffer;
    while(total_sent < length)
    {
        ssize_t sent = send(sock, ptr + total_sent, length - total_sent, 0);
        if(sent < 0)
        {
            return(-1);
        }
        else if(sent == 0)
        {
            return(-1);
        }
        total_sent += sent;
    }
    return total_sent;
}

