#ifndef REQUEST_H
#define REQUEST_H

#include "str_map.h"
#include "http_types.h"

// Only GET is implemented
typedef enum {
    GET,
    POST,
    HEAD
} RequestType;

typedef struct {
    // Type of request
    RequestType type;

    // File requested
    char *requested_file;

    // Header key-value pairs
    StrHashMap *key_value_pairs;

    // Raw header data poitner; used for freeing.
    char *header;
} HttpRequest;

char init_request(HttpRequest *requst);
void free_request(HttpRequest *request);
char read_request(int fd, char **header_buf);
HttpError parse_request(char *request, HttpRequest *req);

#endif
