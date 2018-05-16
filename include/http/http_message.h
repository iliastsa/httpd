#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H

#include "str_map.h"

typedef struct {
    // Header stored in key-value pairs
    StrHashMap *header;
    char *body;
} HttpMessage;

#endif
