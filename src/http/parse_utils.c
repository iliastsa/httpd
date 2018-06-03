#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "parse_utils.h"
#include "request.h"
#include "utils.h"

/*
 * Checks that each \r in the header is followed by a \n.
 *
 * Params:
 * - char *header : The buffer holding the header content.
 *
 * Returns:
 * - -1 if there are unpaired \r or \n.
 * -  0 if the header is well formed.
 */
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

/*
 * Parses the response line (first line of the header), and
 * checks if it is valid.
 *
 * Params:
 * - char *response_line : The buffer holding the response line.
 *
 * Returns:
 * - OK if the response line is legal.
 * - An appropriate HTTP error code, otherwise.
 */
HttpError parse_response_line(char *response_line) {
    char *code, *status, *protcol_version;

    char *save_ptr = NULL;

    // Read type
    protcol_version = strtok_r(response_line, " ", &save_ptr);

    // Read requested file
    code = strtok_r(NULL, " ", &save_ptr);

    // Read protocol version
    status = strtok_r(NULL, " ", &save_ptr);

    // If some of the fields above are missing, it is a bad request
    if (!(code && status && protcol_version))
        return BAD_RESPONSE;

    // Check status
    if (strcmp(status, "OK"))
        return BAD_RESPONSE;

    // Check code
    if (strcmp(code, "200"))
        return BAD_RESPONSE;

    // We only support HTTP/1.1
    if (strcmp(protcol_version, "HTTP/1.1"))
        return BAD_RESPONSE;

    return OK;
}

/*
 * Parses the request line (first line of the header), and
 * checks if it is valid. All useful extracted info is stored in
 * the request struct.
 *
 * Params:
 * - char *request_line   : The buffer holding the request line.
 * - HttpRequest *request : The struct holding all relevant information for
 *                          the request.
 *
 * Returns:
 * - OK if the request line is legal.
 * - An appropriate HTTP error code, otherwise.
 */
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

/*
 * Checks if a string contains the specified any of the
 * specified characters.
 *
 * Params:
 * - char *str         : The string we are interested in.
 * - const char *chars : The string containing all the characters we 
 *                       are looking for.
 *
 * Returns:
 * - 1 if the string contains any of the characters in question.
 * - 0 otherwise.
 */
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

/*
 * Removes all trailing chars in the specifed set from a 
 * string.
 *
 * Params:
 * - char *str         : The string we are interested in.
 * - const char *chars : The string containing all the characters we 
 *                       are looking for.
 *
 * Returns: -
 */
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

/*
 * Removes all leading chars in the specifed set from a 
 * string.
 *
 * Params:
 * - char *str         : The string we are interested in.
 * - const char *chars : The string containing all the characters we 
 *                       are looking for.
 *
 * Returns: -
 */
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

/*
 * Parses a general header line (all lines after the first one are
 * considred general) and checks if it is valid. Since the header
 * fields are of the form <key> : <value>, the header is deserialized
 * and stored into a hash map for future access.
 *
 * Any duplicate fields are considered illegal. Keys and values are
 * converted to lowercase before inserting them into the hash map.
 *
 * Params:
 * - char *request_line     : The buffer holding the header line.
 * - StrHashMap *header_map : The hash map holding all the header
 *                            key-value pairs.
 *
 * Returns:
 * - OK if the header line is legal.
 * - An appropriate HTTP error code, otherwise.
 */
HttpError parse_general_header(char *request_line, StrHashMap *header_map) {
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

    // Convert value to lowercase
    str_to_lowercase(value);

    int status = insert_str_map(header_map, key, value);

    // Something wrong happened
    if (status < 0)
        return UNEXPECTED;
    // Duplicate field (generally not wrong, but for most fields it is)
    else if (status == 0)
        return BAD_RESPONSE;

    return OK;
}
