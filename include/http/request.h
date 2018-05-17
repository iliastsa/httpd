#ifndef REQUEST_H
#define REQUEST_H

#include <inttypes.h>

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

    // Host name
    char *host;
} HttpRequest;

char read_request(int fd, char **header_buf);
HttpRequest *parse_request(char *request);

#endif
