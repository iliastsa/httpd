#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command_manager.h"
#include "server_manager.h"
#include "server_types.h"
#include "network_io.h"
#include "utils.h"

#define CMD_BUF_SZ 512
#define CMD_TIMEOUT  5

/*
 * Reads a command from fd, allocates a buffer, stores the command
 * in it and returns that buffer.
 *
 * Params:
 * - int fd         : The file descriptor we will read from.
 * - char **cmd_buf : The buffer where we will store the command.
 *
 * Returns:
 * - IO_OK if no error occured.
 * - An appropriate IO error code otherwise.
 */
static
int read_command(int fd, char **cmd_buf){
    char chunk_buf[CMD_BUF_SZ];

    // Length of command in bytes
    int command_length = 0;

    int pattern_idx = 0;

    // Buffers used for checking/reallocating
    char *currently_read = NULL;

    int termination_found = 0;

    for (;;) {
        int bytes_read = read_bytes(fd, chunk_buf, CMD_TIMEOUT, CMD_BUF_SZ);

        if (bytes_read <= 0) {
            free(currently_read);
            switch (bytes_read) {
                case 0:
                    return IO_INVALID;
                case IO_TIMEOUT:
                    return IO_TIMEOUT;
                default:
                    return IO_UNEXPECTED;
            }
        }

        command_length += bytes_read;
        char *new_buf = malloc(command_length);

        if (new_buf == NULL) {
            P_DEBUG("Memory allocation failed...\n");
            free(currently_read);
            return IO_UNEXPECTED;
        }

        if (currently_read != NULL)
            memcpy(new_buf, currently_read, command_length - bytes_read);

        int end = 0;
        if ((end=copy_until_delim(chunk_buf, new_buf + command_length - bytes_read, bytes_read, &pattern_idx, "\r\n")) >= 0) {
            new_buf[command_length - bytes_read + end - 1] = '\0';
            termination_found = 1;
        }

        free(currently_read);
        currently_read = new_buf;

        if (termination_found)
            break;
    }

    *cmd_buf = currently_read;
    return IO_OK;
}

/*
 * Handler for the STATS command
 *
 * Params:
 * - int fd                  : The file descriptor we will respond to.
 * - ServerResources *server : The struct containing all server resources.
 *
 * Returns: -
 */
static
void cmd_stats(int fd, ServerResources *server) {
    static const char *msg_fmt = 
    "Server up for %02llu:%02llu:%02llu.%03llu, served %llu pages, %llu bytes\r\n";

    // Buffer where our message is stored
    char *msg = NULL;

    // Get current time
    struct timeval t_now;
    if (gettimeofday(&t_now, NULL) < 0)
        return;

    // Calculate time difference
    TimeFormat t_diff;
    diff_time(&server->t_start, &t_now, &t_diff);

    // Get a instance of the server stats (needs locking to ensure atomicity)
    ServerStats stats;
    get_stats_instance(&server->stats, &stats);

    // Calculate required buffer length
    int len = snprintf(NULL, 0, msg_fmt, t_diff.hours, 
                                         t_diff.minutes, 
                                         t_diff.seconds, 
                                         t_diff.milisec,
                                         stats.page_count,
                                         stats.byte_count);

    if (len < 0) { 
        P_DEBUG("sprintf failed while stats string\n");
        goto EXIT;
    }

    // Allocate buffer
    msg = malloc (len + 1);

    if (msg == NULL) {
        P_ERR("Malloc failed for stats string", errno);
        goto EXIT;
    }

    len = snprintf(msg, len + 1, msg_fmt, t_diff.hours, 
                                          t_diff.minutes, 
                                          t_diff.seconds, 
                                          t_diff.milisec,
                                          stats.page_count,
                                          stats.byte_count);

    if (len < 0) { 
        P_DEBUG("sprintf failed while stats string\n");
        goto EXIT;
    }

    // Send message
    write_bytes(fd, msg, CMD_TIMEOUT, len);

EXIT:
    free(msg);
}

/*
 * Reads the command, analyzes it and calls the appropriate handler.
 *
 * Params:
 * - int fd                  : The file descriptor we will read from and respond to.
 * - ServerResources *server : The struct containing all server resources.
 * - int parent_in[2]        : An array holding the pipe fds for parent input.
 * - int child_in[2]         : An array holding the pipe fds for child input.
 *
 * Returns: -
 */
int accept_command(int fd, ServerResources *server) {
    char *cmd = NULL;

    int err = CMD_UNKNOWN;
    if ((err = read_command(fd, &cmd)) != IO_OK)
        goto EXIT;

    if (!strcmp(cmd, "SHUTDOWN")) {
        err = CMD_SHUTDOWN;
    } else if (!strcmp(cmd, "STATS")) {
        cmd_stats(fd, server);
        err = CMD_STATS;
    } else if (!strcmp(cmd, "KILLT")) {
        pthread_cancel(server->thread_pool->threads[0]);
    } else {
        err = CMD_UNKNOWN;
    }

EXIT:
    free(cmd);
    close(fd);
    return err;
}
