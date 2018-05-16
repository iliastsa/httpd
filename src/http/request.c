#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

#define CHUNK_SZ 1024

static
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

static
int copy_until_end(char *source, char *dest, int sz, int *offset){
    // Used to recognise the end of the header
    const char *end_of_header = "\r\n\r\n";
    int pattern_end = strlen(end_of_header);

    for (int i = 0; i < sz; ++i) {
        dest[i] = source[i];

        if (source[i] == end_of_header[*offset]) {
            P_DEBUG("Found symbol num %d from end of header delimiter\n", *offset);
            (*offset)++;
        }
        else
            *offset = 0;

        if (*offset == pattern_end)
            return i;
    }

    return -1;
}

/*
 * Reads HTTP request, and ignores the body portion. This means that
 * reads are issued until \r\n\r\n is found.
 *
 */
char read_request(int fd, char **header_buf) {
    char chunk_buf[CHUNK_SZ];

    // Length of the header in bytes
    int header_length = 0;

    int pattern_idx = 0;

    // Buffers used for checking/reallocating
    char *currently_read = NULL;

    int termination_found = 0;

    for (;;) {
        int bytes_read = read_bytes(fd, chunk_buf, CHUNK_SZ);

        if (bytes_read < 0) {
            free(currently_read);
            return -1;
        }

        // We need to grow the header, allocate space to accomodate the new header
        header_length += bytes_read;
        char *new_buf = malloc(header_length);

        if (new_buf == NULL) {
            P_DEBUG("Memory allocation failed...\n");
            free(currently_read);
        }

        if (currently_read != NULL)
            memcpy(new_buf, currently_read, header_length - bytes_read);

        int end = 0;
        if ((end=copy_until_end(chunk_buf, new_buf + header_length - bytes_read, bytes_read, &pattern_idx)) >= 0) {
            P_DEBUG("Found end of request sequence at byte %d\n", header_length - bytes_read + end);
            P_DEBUG("Setting 2 bytes from the end to NULL, to signify string end\n");

            new_buf[header_length - bytes_read + end - 2] = '\0';
            termination_found = 1;
        }

        free(currently_read);
        currently_read = new_buf;

        if (termination_found)
            break;
    }

    *header_buf = currently_read;
    return 1;
}
