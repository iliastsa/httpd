#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "parse_utils.h"
#include "http_types.h"
#include "network_io.h"
#include "request.h"
#include "utils.h"

#define CHUNK_SZ 1024

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
        default:
            break;
    }
}

/*
 * Initializes a new request struct.
 *
 * Params:
 * - HttpRequest *request : The request to be initalized.
 *
 * Returns:
 * -  0 if no error occured.
 * - -1 otherwise.
 */
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

/*
 * Frees all memory occupied by the request.
 *
 * Params:
 * - HttpRequest *request : The request to be freed.
 *
 * Returns: -
 */
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
 * Params:
 * - int fd            : The file descriptor we are reading from.
 * - char **header_buf : The buffer that will be allocated for the header
 *                       to be stored in.
 *
 * Returns:
 * - OK if no error occured
 * - An appropriate HTTP error code otherwise.
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
        if ((end=copy_until_delim(chunk_buf, new_buf + header_length - bytes_read, bytes_read, &pattern_idx, "\r\n\r\n")) >= 0) {

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

/*
 * Checks if a key-value pair exists in the hash map.
 *
 * Params:
 * - StrHashMap *header : The hash map containing the header key-value
 *                        pairs.
 *
 * Returns:
 * -  0 if the assertion was true
 * - -1 otherwise
 */
static
int assert_header(StrHashMap *header, char * key, char *val) {
    char *ret = lookup_str_map(header, key);

    if (ret == NULL || (val != NULL && strcmp(ret, val)))
        return -1;

    return 0;
}

/*
 * Performs a series of checks on the header key-value pairs.
 *
 * Params:
 * - StrHashMap *header : The hash map containing the header key-value
 *                        pairs.
 *
 * Returns:
 * - OK if the header content passed all checks.
 * - BAD_REQUEST if any check failed.
 */
HttpError check_request_header(StrHashMap *header) {
    // Check connection field
    // Throw error if it does not exist or value is different than keep-alive
    // and closed.
    if (assert_header(header, "connection", "keep-alive") &&
        assert_header(header, "connection", "close"))
        return BAD_REQUEST;

    // More checks can be added here, the code is very modular.
    // For the purpose of this exercise, only the connection field 
    // is checked.
    
    return OK;
}

/*
 * Parses the request header, and checks if everything is legal.
 *
 * Params:
 * - char *request    : The buffer holding the request header.
 * - HttpRequest *req : The request struct where the parsed info
 *                      will be stored.
 *
 * Returns:
 * - OK if the header parse was successful.
 * - An appropriate HTTP error code otherwise.
 */
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
        if ((err = parse_general_header(line_ptr, req->key_value_pairs)) != OK)
            return err;
    }

    print_str_map(req->key_value_pairs);

    return OK;
}
