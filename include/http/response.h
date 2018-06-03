#ifndef RESPONSE_H
#define RESPONSE_H

#include "http_types.h"
#include "str_map.h"

typedef struct {
    // File requested
    char *requested_file;

    // Content length
    int content_length;

    // Header key-value pairs
    StrHashMap *key_value_pairs;

    // Buffers holding header and content data
    char *header;
    char *content;
} HttpResponse;

char init_response(HttpResponse *response);
void free_response(HttpResponse *response);
HttpError get_response(int fd, HttpResponse *response);

#endif
