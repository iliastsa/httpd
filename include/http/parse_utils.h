#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include "request.h"

HttpError parse_general_header(char *request_line, HttpRequest *request);
HttpError parse_request_line(char *request_line, HttpRequest *request);
char check_line_termination(char *header);

#endif
