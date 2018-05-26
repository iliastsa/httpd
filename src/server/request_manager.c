#define _GNU_SOURCE
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>

#include "server_manager.h"
#include "server_types.h"
#include "http_types.h"
#include "network_io.h"
#include "request.h"
#include "utils.h"

extern const char * const response_messages[];

static
void write_err_response(int fd, HttpError err) {
    if (err == OK) {
        P_DEBUG("Error write function called with OK error code\n");
        return;
    }

    const char *format = response_messages[err];

    // Get date
    time_t t_now      = time(NULL);
    struct tm t_data;
    gmtime_r(&t_now, &t_data);

    char date[512];
    strftime(date, 512, "%a, %d %b %Y %H:%M:%S %Z", &t_data);

    int len = snprintf(NULL, 0, format, date);
    
    if (len < 0) { 
        P_DEBUG("sprintf failed while writing the date\n");
        return;
    }

    char *msg = malloc(len + 1);

    if (msg == NULL) {
        P_ERR("Malloc failed for date string", errno);
        goto EXIT;
    }

    len = snprintf(msg, len + 1, format, date);
    
    if (len < 0) { 
        P_DEBUG("sprintf failed while writing the date\n");
        goto EXIT;
    }

    write_bytes(fd, msg, HTTP_TIMEOUT, len);

EXIT:
    free(msg);
}

static 
void write_ok_response(int fd, char *file, ServerStats *stats) {
    const char *format = response_messages[OK];

    char *msg  = NULL;

    // Get date
    time_t t_now      = time(NULL);
    struct tm t_data;
    gmtime_r(&t_now, &t_data);

    char date[512];
    strftime(date, 512, "%a, %d %b %Y %H:%M:%S %Z", &t_data);

    // Get content length
    long sz = 0;
    if (count_bytes(file, &sz) < 0) {
        P_DEBUG("Size counting failed\n");
        return;
    }

    int len = snprintf(NULL, 0, format, date, sz);
    
    if (len < 0) { 
        P_DEBUG("sprintf failed while writing the date\n");
        return;
    }

    msg = malloc(len + 1);

    if (msg == NULL) {
        P_ERR("Malloc failed for date string", errno);
        return;
    }

    len = snprintf(msg, len + 1, format, date, sz);
    
    if (len < 0) { 
        P_DEBUG("sprintf failed while writing the date\n");
        goto EXIT;
    }

    // If header write and html file write suceeded, update the stats
    if ((write_bytes(fd, msg, HTTP_TIMEOUT, len) == IO_OK) && (write_file(fd, file, HTTP_TIMEOUT) == IO_OK))
        update_stats(stats, sz);

EXIT:
    free(msg);
}

static 
void write_response(int fd, HttpError err, char *full_path, ServerStats *stats) {
    if (err == OK) 
        write_ok_response(fd, full_path, stats);
    else
        write_err_response(fd, err);
}

static
HttpError check_file_access(char *file, char *root_dir, char **full_path) {
    P_DEBUG("File : (%s) Root : (%s)\n", file, root_dir);
    char *expanded_path = realpath(file, NULL);

    if (expanded_path == NULL) {
        switch (errno) {
            case ENOENT:
            case ENOTDIR:
                return NOT_FOUND;
            case EACCES:
                return FORBIDDEN;
            default:
                return UNEXPECTED;
        }
    }

    *full_path = expanded_path;

    struct stat f_stats;

    if (stat(expanded_path, &f_stats) < 0) {
        switch (errno) {
            case ENOTDIR:
            case ENOENT:
                return NOT_FOUND;
            case EACCES:
                return FORBIDDEN;
            default:
                return UNEXPECTED;
        }
    }

    // Check if the file is a regular file
    if (S_ISDIR(f_stats.st_mode))
        return NOT_FOUND;

    // Check if the file is readable
    if (!(f_stats.st_mode & S_IRUSR))
        return FORBIDDEN;

    int root_len = strlen(root_dir);
    int file_len = strlen(expanded_path);

    if (root_len > file_len)
        return FORBIDDEN;

    for (int i = 0; i < root_len; ++i)
        if (root_dir[i] != expanded_path[i])
            return FORBIDDEN;

    return OK;
}

void accept_http(void *arg) {
    int fd             = ((AcceptArgs*)arg)->fd;
    char *root_dir     = ((AcceptArgs*)arg)->root_dir;
    ServerStats *stats = ((AcceptArgs*)arg)->stats;

    char *header   = NULL;

    char *file_full_path = NULL;
    char *file_w_root    = NULL;

    P_DEBUG("Got fd %d\n", fd);

    HttpRequest *request = malloc(sizeof(HttpRequest));

    HttpError err = UNEXPECTED;

    if (request == NULL)
        goto CLOSE_CONNECTION;

    if (init_request(request) < 0)
        goto CLOSE_CONNECTION;
    
    if ((err = read_request(fd, &header)) != OK)
        goto CLOSE_CONNECTION;

    if ((err = parse_request(header, request)) != OK)
        goto CLOSE_CONNECTION;

    if ((err = check_request_header(request->key_value_pairs)) != OK)
        goto CLOSE_CONNECTION;

    int root_len = strlen(root_dir);
    int file_len = strlen(request->requested_file);

    // Requested file is <root_dir>/file_name
    file_w_root = malloc(root_len + file_len +1);

    err = UNEXPECTED;

    if (file_w_root == NULL)
        goto CLOSE_CONNECTION;

    // Clear buffer
    memset(file_w_root, 0, root_len + file_len + 1);

    // Copy root
    memcpy(file_w_root, root_dir, root_len);

    // Copy file
    memcpy(file_w_root + root_len, request->requested_file, file_len);

    if ((err = check_file_access(file_w_root, root_dir, &file_full_path)) != OK)
        goto CLOSE_CONNECTION;

CLOSE_CONNECTION:
    write_response(fd, err, file_full_path, stats);
    free_request(request);
    free(file_w_root);
    free(file_full_path);
    close(fd);
}
