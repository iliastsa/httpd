#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "parse_utils.h"
#include "http_types.h"
#include "network_io.h"
#include "request.h"
#include "utils.h"

#define CHUNK_SZ 1024

static
int copy_until_end(char *source, char *dest, int sz, int *offset){
    // Used to recognise the end of the header
    const char *end_of_header = "\r\n\r\n";
    int pattern_end = strlen(end_of_header);

    for (int i = 0; i < sz; ++i) {
        dest[i] = source[i];

        if (source[i] == end_of_header[*offset]) {
            //P_DEBUG("Found symbol num %d from end of header delimiter\n", *offset);
            (*offset)++;
        }
        else
            *offset = 0;

        if (*offset == pattern_end)
            return i;
    }

    return -1;
}

static
void print_debug(HttpError error) {
    switch (error) {
        case BAD_REQUEST:
            P_DEBUG("Bad request\n");
            break;
        case NOT_IMPLEMENTED:
            P_DEBUG("Method not implemented\n");
            break;
        case VERSION_NOT_SUPPORTED:
            P_DEBUG("HTTP version not supported\n");
            break;
        case UNEXPECTED:
            P_DEBUG("HTTP version not supported\n");
            break;
        case FORBIDDEN:
            P_DEBUG("Forbidden request\n");
            break;
        case NOT_FOUND:
            P_DEBUG("Requested file not found\n");
            break;
        case TIMEOUT:
            P_DEBUG("Request timed out\n");
            break;
        case OK:
            P_DEBUG("Request line OK!\n");
            break;
    }
}

char init_request(HttpRequest *request){
    request->key_value_pairs = NULL;
    request->requested_file  = NULL;
    request->header          = NULL;

    StrHashMap *map = (StrHashMap*) malloc(sizeof(StrHashMap));

    if (map == NULL) {
        P_DEBUG("Memory allocation for HTTP request failed\n");
        return -1;
    }

    // Initiaize map
    init_str_map(map);

    request->key_value_pairs = map;

    return 0;
}

void free_request(HttpRequest *request) {
    if (request == NULL)
        return;

    // Free header buffer
    free(request->header);

    // Free hashmap
    free_str_map(request->key_value_pairs, 0);

    free(request);
}

/*
 * Reads HTTP request, and ignores the body portion. This means that
 * reads are issued until \r\n\r\n is found.
 *
 */
HttpError read_request(int fd, char **header_buf) {
    char chunk_buf[CHUNK_SZ];

    // Length of the header in bytes
    int header_length = 0;

    int pattern_idx = 0;

    // Buffers used for checking/reallocating
    char *currently_read = NULL;

    int termination_found = 0;

    for (;;) {
        int bytes_read = read_bytes(fd, chunk_buf, HTTP_TIMEOUT, CHUNK_SZ);

        if (bytes_read <= 0) {
            free(currently_read);
            switch (bytes_read) {
                case 0:
                    return BAD_REQUEST;
                case IO_TIMEOUT:
                    return TIMEOUT;
                default:
                    return UNEXPECTED;
            }
        }

        // We need to grow the header, allocate space to accomodate the new header
        header_length += bytes_read;
        char *new_buf = malloc(header_length);

        if (new_buf == NULL) {
            P_DEBUG("Memory allocation failed...\n");
            free(currently_read);
            return UNEXPECTED;
        }

        if (currently_read != NULL)
            memcpy(new_buf, currently_read, header_length - bytes_read);

        int end = 0;
        if ((end=copy_until_end(chunk_buf, new_buf + header_length - bytes_read, bytes_read, &pattern_idx)) >= 0) {
            P_DEBUG("Found end of request sequence at byte %d\n", header_length - bytes_read + end);
            P_DEBUG("Setting 2 bytes from the end to NULL, to signify string end\n");

            new_buf[header_length - bytes_read + end - 1] = '\0';
            termination_found = 1;
        }

        free(currently_read);
        currently_read = new_buf;

        if (termination_found)
            break;
    }

    *header_buf = currently_read;
    return OK;
}

HttpError parse_request(char *request, HttpRequest *req){
    // Store request into header var of struct
    req->header = request;

    // Check if lines can be safely tokenized
    if(check_line_termination(request)) {
        P_DEBUG("The header has malformed lines\n");
        return BAD_REQUEST;
    }
    else
        P_DEBUG("The header is ok!\n");

    char *start;
    char *save_ptr;

    // Start line seperation here
    for (start = request; ; start = NULL) {
        HttpError err;

        // Read line
        char *line_ptr = strtok_r(start, "\r\n", &save_ptr);
        if (line_ptr == NULL)
            break;

        // First line is special, parse with appropriate function
        if (start != NULL) {
            if ((err = parse_request_line(line_ptr, req)) != OK)
                return err;
            else
                continue;
        }

        // Each line is a key-value pair, that is handled in a similar way
        if ((err = parse_general_header(line_ptr, req)) != OK)
            return err;
    }

    return OK;
}
