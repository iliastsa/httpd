#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

#include "str_map.h"

typedef enum {
    BAD_REQUEST,
    NOT_IMPLEMENTED,
    VERSION_NOT_SUPPORTED,
    UNEXPECTED,
    OK
} HttpError;

typedef struct {
    // Header stored in key-value pairs
    StrHashMap *header;
    char *body;
} HttpMessage;

#endif
