#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "parse_utils.h"
#include "request.h"
#include "utils.h"

char check_line_termination(char *header) {
    int len = strlen(header);

    // Iterate over all characters in the header
    for (int i = 0; i < len; ++i)
        // Catch any unpaired \r or \n in the header
        if ( (header[i] == '\r' && header[i+1] != '\n') ||
             (i == 0 && header[i] == '\n') ||
             (i  > 0 && header[i] == '\n' && header[i-1] != '\r')
           )
            return -1;

    return 0;
}

HttpError parse_request_line(char *request_line, HttpRequest *request) {
    char *type, *file, *protcol_version;

    char *save_ptr;

    // Read type
    type = strtok_r(request_line, " ", &save_ptr);

    // Read requested file
    file = strtok_r(NULL, " ", &save_ptr);

    // Read protocol version
    protcol_version = strtok_r(NULL, " ", &save_ptr);

    // If some of the fields above are missing, it is a bad request
    if (!(type && file && protcol_version))
        return BAD_REQUEST;

    // If there is an extra field, it is a bad request
    if (strtok_r(NULL, " ", &save_ptr))
        return BAD_REQUEST;

    // We only implement GET request
    if (!strcmp(type, "GET"))
        request->type = GET;
    else
        return NOT_IMPLEMENTED;

    // We only support HTTP/1.1
    if (strcmp(protcol_version, "HTTP/1.1"))
        return VERSION_NOT_SUPPORTED;

    // If all went well, store request file
    request->requested_file = file;

    return OK;
}

static
char contains_chars(char *str, const char *chars) {
    size_t len = strlen(str);
    size_t char_len = strlen(chars);

    for (size_t i = 0; i < len; ++i)
        for (size_t j = 0; j < char_len; ++j)
            if (str[i] == chars[j])
                return 1;

    return 0;
}

static
void strip_trailing_chars(char *str, const char *chars) {
    size_t len = strlen(str);
    size_t char_len = strlen(chars);

    if (char_len == 0 || len == 0)
        return;

    // Scan characters from back to front
    for (size_t i = 0; i < len; ++i) {
        char found = 0;

        // Replace any matching characters
        for (size_t j = 0; j < char_len; ++j) {
            if (str[len - 1  - i] == chars[j]) {
                str[len - 1 - i] = '\0';
                found = 1;
                break;
            }
        }

        // Stop on first character found that is not in chars
        if (!found)
            break;
    }
}

static
void strip_leading_chars(char **src, const char *chars) {
    size_t len = strlen(*src);
    size_t char_len = strlen(chars);

    if (char_len == 0 || len == 0)
        return;

    // Keep track of the start
    char *str = *src;

    // Scan characters from back to front
    for (size_t i = 0; i < len; ++i) {
        char found = 0;

        // Replace any matching characters
        for (size_t j = 0; j < char_len; ++j) {
            if (str[i] == chars[j]) {
                // Advance pointer on place further
                (*src)++;
                found = 1;
                break;
            }
        }

        // Stop on first character found that is not in chars
        if (!found)
            break;
    }
}

HttpError parse_general_header(char *request_line, HttpRequest *request) {
    char *key, *value;

    char *save_ptr;

    // HTTP Header lines must contain a ':' as a seperator
    if (!contains_chars(request_line, ":"))
        return BAD_REQUEST;

    // Read key portion
    key = strtok_r(request_line, ":", &save_ptr);

    // Read value portion
    value = strtok_r(NULL, "\r", &save_ptr);

    // If key or value is NULL
    if (!(key && value))
        return BAD_REQUEST;

    // Line must be in the format : ^<key_no_ws>:
    if (contains_chars(key, " \t"))
        return BAD_REQUEST;

    // Remove any trailing whitespace from value
    strip_trailing_chars(value, " \t");

    // Remove any leading whitespace from value
    strip_leading_chars(&value, " \t");

    // If key was only whitespace, throw error
    if (strlen(value) == 0)
        return BAD_REQUEST;

    // Convert key to lowercase
    str_to_lowercase(key);

    int status = insert_str_map(request->key_value_pairs, key, value);

    // Something wrong happened
    if (status < 0)
        return UNEXPECTED;
    // Duplicate field (generally not wrong, but for most fields it is)
    else if (status == 0)
        return BAD_REQUEST;

    return OK;
}
