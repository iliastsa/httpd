#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

#include "str_map.h"

#define HTTP_TIMEOUT 5

typedef enum {
    BAD_REQUEST = 0,
    NOT_IMPLEMENTED,
    VERSION_NOT_SUPPORTED,
    UNEXPECTED,
    NOT_FOUND,
    FORBIDDEN,
    TIMEOUT,
    OK
} HttpError;

typedef struct {
    // Header stored in key-value pairs
    StrHashMap *header;
    char *body;
} HttpMessage;

#endif
