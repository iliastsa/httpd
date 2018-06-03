#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include "str_map.h"
#include "response.h"
#include "request.h"

HttpError parse_general_header(char *request_line, StrHashMap *header_map);
HttpError parse_request_line(char *request_line, HttpRequest *request);
HttpError parse_response_line(char *request_line);
char check_line_termination(char *header);

#endif
