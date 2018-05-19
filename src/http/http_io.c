#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

#define FILE_BUF_SZ 1024

int read_bytes(int fd, char *buf, size_t n_bytes) {
    int bytes_read;

    for (;;) {
        bytes_read = read(fd, buf, n_bytes);

        // Check for errors and EOF
        if (bytes_read < 0)
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return -1;
            }
        else
            return bytes_read;
    }
}

int write_bytes(int fd, char *buf, size_t n_bytes) {
    ssize_t total_bytes_written = 0;
    ssize_t remaining_bytes     = n_bytes;

    for (;;) {
        ssize_t bytes_written = write(fd, buf + total_bytes_written, remaining_bytes);

        if (bytes_written < 0) {
            switch (errno) {
                case EINTR:
                    continue;
                default:
                    return -1;
            }
        }

        total_bytes_written += bytes_written;
        remaining_bytes     -= bytes_written;

        if (remaining_bytes == 0)
            break;
    }

    return 0;
}

int write_file(int fd, char *filepath) {
    int file = open(filepath, O_RDONLY);

    if (file < 0)
        return -1;

    char buf[FILE_BUF_SZ];

    // Read in chunks and write them to the fd
    for (;;) {
        int bytes_read = read_bytes(file, buf, FILE_BUF_SZ);

        // If an error occured, stop writting
        if (bytes_read < 0) {
            close(file);
            return -1;
        }
        // We reached EOF
        else if (bytes_read == 0)
            break;

        // Whatever you read above, write it
        int bytes_written = write_bytes(fd, buf, bytes_read);

        // If an error occured during write, stop writting
        if (bytes_written < 0) {
            close(file);
            return -1;
        }
    }
    
    close(file);
    return 0;
}
