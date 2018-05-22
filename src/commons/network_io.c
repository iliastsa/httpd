#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include "network_io.h"
#include "utils.h"

#define FILE_BUF_SZ 1024
#define ONE_SECOND  1000

int read_bytes(int fd, char *buf, int timeout, size_t n_bytes) {
    int bytes_read;

    struct pollfd fd_info;

    fd_info.fd = fd;
    fd_info.events = POLLIN;

    for (;;) {
        int status = poll(&fd_info, 1, timeout * ONE_SECOND);

        // Timeout
        if (status == 0) {
            P_DEBUG("Read timed out\n");
            return IO_TIMEOUT;
        }

        // Error in poll
        if (status == -1)
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return IO_UNEXPECTED;
            }

        // Read bytes from file descriptor
        bytes_read = read(fd, buf, n_bytes);

        // Check for errors and EOF
        if (bytes_read < 0)
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return IO_UNEXPECTED;
            }
        else
            return bytes_read;
    }
}

int write_bytes(int fd, char *buf, int timeout, size_t n_bytes) {
    ssize_t total_bytes_written = 0;
    ssize_t remaining_bytes     = n_bytes;

    struct pollfd fd_info;

    fd_info.fd = fd;
    fd_info.events = POLLOUT;

    for (;;) {
        int status = poll(&fd_info, 1, timeout * ONE_SECOND);

        // Timeout
        if (status == 0) {
            P_DEBUG("Read timed out\n");
            return IO_TIMEOUT;
        }

        // Error in poll
        if (status == -1)
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return IO_UNEXPECTED;
            }

        ssize_t bytes_written = write(fd, buf + total_bytes_written, remaining_bytes);

        if (bytes_written < 0) {
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return IO_UNEXPECTED;
            }
        }

        total_bytes_written += bytes_written;
        remaining_bytes     -= bytes_written;

        if (remaining_bytes == 0)
            break;
    }

    return IO_OK;
}

int write_file(int fd, char *filepath, int timeout) {
    int file = open(filepath, O_RDONLY);

    if (file < 0)
        return IO_UNEXPECTED;

    char buf[FILE_BUF_SZ];

    // Read in chunks and write them to the fd
    for (;;) {
        int bytes_read = read_bytes(file, buf, -1, FILE_BUF_SZ);

        // If an error occured, stop writting
        if (bytes_read < 0) {
            close(file);
            return bytes_read;
        }
        // We reached EOF
        else if (bytes_read == 0)
            break;

        // Whatever you read above, write it
        int bytes_written = write_bytes(fd, buf, timeout, bytes_read);

        // If an error occured during write, stop writting
        if (bytes_written < 0) {
            close(file);
            return IO_UNEXPECTED;
        }
    }
    
    close(file);
    return IO_OK;
}
