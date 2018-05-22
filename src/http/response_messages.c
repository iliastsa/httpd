#include "http_types.h"

const char * const response_messages[] =
{
    // Bad Request
    [BAD_REQUEST] = 
    "HTTP/1.1 400 Bad Request\r\n"
    "Date: %s\r\n"
    "Content-Length: 24\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Bad Request</html>",

    // Not Implemented
    [NOT_IMPLEMENTED] = 
    "HTTP/1.1 501 Not Implemented\r\n"
    "Date: %s\r\n"
    "Content-Length: 28\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Not Implemeneted</html>",

    // Version Not Supported
    [VERSION_NOT_SUPPORTED] = 
    "HTTP/1.1 505 Version Not Supported\r\n"
    "Date: %s\r\n"
    "Content-Length: 34\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Version Not Supported</html>",

    // Unexpected Error
    [UNEXPECTED] = 
    "HTTP/1.1 505 Internal Server Error\r\n"
    "Date: %s\r\n"
    "Content-Length: 34\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Internal Server Error</html>",

    // Not Found
    [NOT_FOUND] =
    "HTTP/1.1 404 Not Found\r\n"
    "Date: %s\r\n"
    "Content-Length: 22\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Not Found</html>",

    // Forbidden
    [FORBIDDEN] =
    "HTTP/1.1 403 Forbidden\r\n"
    "Date: %s\r\n"
    "Content-Length: 22\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Forbidden</html>",

    // Timeout
    [TIMEOUT] =
    "HTTP/1.1 408 Request Timeout\r\n"
    "Date: %s\r\n"
    "Content-Length: 28\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
    "<html>Request Timeout</html>",

    // OK
    [OK] = 
    "HTTP/1.1 200 OK\r\n"
    "Date: %s\r\n"
    "Content-Length: %ld\r\n"
    "Content-Type: text/html\r\n"
    "Connection: Closed\r\n"
    "\r\n"
};
