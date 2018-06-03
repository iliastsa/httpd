#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "parse_utils.h"
#include "http_types.h"
#include "network_io.h"
#include "response.h"
#include "utils.h"

#define CHUNK_SZ 1024

static HttpError parse_response(char *response, HttpResponse *res);
static HttpError check_response_header(StrHashMap *header);

static
void print_debug(HttpError error) {
    switch (error) {
        case BAD_REQUEST:
            P_DEBUG("Bad response\n");
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
            P_DEBUG("Forbidden response\n");
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

static
int get_response_content_len(HttpResponse *response) {
    char *ret = lookup_str_map(response->key_value_pairs, "content-length");

    if (ret == NULL)
        return -1;

    char *end;

    int val = strtol(ret, &end, 10);

    if (*end != '\0' || val < 0)
        return -1;

    response->content_length = val;

    return 0;
}

/*
 * Initializes a new response struct.
 *
 * Params:
 * - HttpResponse *response : The response to be initalized.
 *
 * Returns:
 * -  0 if no error occured.
 * - -1 otherwise.
 */
char init_response(HttpResponse *response){
    response->key_value_pairs = NULL;
    response->requested_file  = NULL;
    response->content         = NULL;
    response->header          = NULL;

    StrHashMap *map = (StrHashMap*) malloc(sizeof(StrHashMap));

    if (map == NULL) {
        P_DEBUG("Memory allocation for HTTP response failed\n");
        return -1;
    }

    // Initiaize map
    init_str_map(map);

    response->key_value_pairs = map;

    return 0;
}

/*
 * Frees all memory occupied by the response.
 *
 * Params:
 * - HttpResponse *response : The response to be freed.
 *
 * Returns: -
 */
void free_response(HttpResponse *response) {
    if (response == NULL)
        return;

    // Free header buffer
    free(response->header);

    // Free content buffer
    free(response->content);

    // Free hashmap
    free_str_map(response->key_value_pairs, 0);

    free(response);
}

/*
 * Reads HTTP response, both the header and the body.
 *
 * Params:
 * - int fd                 : The file descriptor we are reading from.
 * - HttpResponse *response : The struct where all the response info
 *                            will be stored in.
 *
 * Returns:
 * - OK if no error occured
 * - An appropriate HTTP error code otherwise.
 */
HttpError get_response(int fd, HttpResponse *response) {
    char chunk_buf[CHUNK_SZ];

    // Length of the header in bytes
    int header_length = 0;

    // Length of content in bytes
    int over_read_bytes = 0;

    int pattern_idx = 0;

    // Buffers used for checking/reallocating
    char *currently_read = NULL;
    char *content_buf    = NULL;
    char *over_read      = NULL;

    int termination_found = 0;

    for (;;) {
        int bytes_read = read_bytes(fd, chunk_buf, HTTP_TIMEOUT, CHUNK_SZ);

        if (bytes_read <= 0) {
            free(currently_read);
            switch (bytes_read) {
                case 0:
                    return BAD_RESPONSE;
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

            int leftover_bytes = bytes_read - end - 1;

            // If bytes read is more than end + 1, there is some content to be copied over
            if (leftover_bytes > 0) {
                over_read = malloc (leftover_bytes);

                if (over_read == NULL) {
                    P_DEBUG("Memory allocation failed...\n");
                    free(currently_read);
                    return UNEXPECTED;
                }

                memcpy(over_read, chunk_buf + end + 1, leftover_bytes);
                over_read_bytes += leftover_bytes;
            }
        }

        free(currently_read);
        currently_read = new_buf;

        if (termination_found)
            break;
    }


    // If we reached here, the header has been read, time to parse it
    response->header = currently_read;

    HttpError err;
    if ((err = parse_response(currently_read, response)) != OK) {
        free(over_read);
        return err;
    }

    if ((err = check_response_header(response->key_value_pairs)) != OK) {
        free(over_read);
        return err;
    }

    if (get_response_content_len(response)) {
        free(over_read);
        return BAD_RESPONSE;
    }

    // Allocate buffer to store response content
    content_buf = malloc(response->content_length + 1);

    if (content_buf == NULL) {
        free(over_read);
        return UNEXPECTED;
    }

    // Null terminate content buffer
    content_buf[response->content_length] = '\0';

    // Store it in the response header field
    response->content = content_buf;

    // There is content_length bytes read
    int content_left = response->content_length;

    if (over_read != NULL) {
        memcpy(content_buf, over_read, over_read_bytes);
        content_left -= over_read_bytes;
        content_buf  += over_read_bytes;
        free(over_read);
    }

    // Read content_length bytes
    while (content_left > 0) {
        int bytes_read = read_bytes(fd, content_buf, HTTP_TIMEOUT, content_left);

        if (bytes_read <= 0) {
            switch (bytes_read) {
                case 0:
                    return BAD_REQUEST;
                case IO_TIMEOUT:
                    return TIMEOUT;
                default:
                    return UNEXPECTED;
            }
        }

        content_left -= bytes_read;
        content_buf  += bytes_read;
    }

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
 * Checks if a key-value pair exists in the hash map, where value
 * is an integer.
 *
 * In this special variation of the assertion function, we are only
 * interested to see if the value is an positive integer.
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
int assert_header_int(StrHashMap *header, char *key) {
    char *ret = lookup_str_map(header, key);

    if (ret == NULL)
        return -1;

    char *end;

    int val = strtol(ret, &end, 10);

    if (*end != '\0' || val < 0)
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
 * - BAD_RESPONSE if any check failed.
 */
HttpError check_response_header(StrHashMap *header) {
    // Check connection field
    // Throw error if it does not exist or value is different than keep-alive
    // and closed.
    if (assert_header(header, "connection", "keep-alive") &&
        assert_header(header, "connection", "close"))
        return BAD_RESPONSE;

    if (assert_header_int(header, "content-length"))
        return BAD_RESPONSE;
    
    return OK;
}

/*
 * Parses the response header, and checks if everything is legal.
 *
 * Params:
 * - char *requeust   : The buffer holding the response header.
 * - HttpRequest *req : The response struct where the parsed info
 *                      will be stored.
 *
 * Returns:
 * - OK if the header parse was successful.
 * - An appropriate HTTP error code otherwise.
 */
static
HttpError parse_response(char *response, HttpResponse *res){
    // Store response into header var of struct
    res->header = response;

    // Check if lines can be safely tokenized
    if(check_line_termination(response)) {
        P_DEBUG("The header has malformed lines\n");
        return BAD_RESPONSE;
    }

    //P_DEBUG("The header is ok!\n");

    char *start;
    char *save_ptr;

    // Start line seperation here
    for (start = response; ; start = NULL) {
        HttpError err;

        // Read line
        char *line_ptr = strtok_r(start, "\r\n", &save_ptr);
        if (line_ptr == NULL)
            break;

        // First line is special, parse with appropriate function
        if (start != NULL) {
            if ((err = parse_response_line(line_ptr)) != OK)
                return err;
            else
                continue;
        }

        // Each line is a key-value pair, that is handled in a similar way
        if ((err = parse_general_header(line_ptr, res->key_value_pairs)) != OK)
            return err;
    }

    return OK;
}
