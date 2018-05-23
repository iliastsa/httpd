#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command_manager.h"
#include "server_types.h"
#include "network_io.h"
#include "utils.h"

#define CMD_BUF_SZ 512
#define CMD_TIMEOUT 10

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
            P_DEBUG("Found end of command sequence at byte %d\n", command_length - bytes_read + end);
            P_DEBUG("Setting 2 bytes from the end to NULL, to signify string end\n");

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

static
void cmd_stats(int fd, ServerResources *server) {
}

int accept_command(int fd, ServerResources *server) {
    char *cmd = NULL;

    int err;
    if ((err = read_command(fd, &cmd)) != IO_OK)
        goto EXIT;

    if (!strcmp(cmd, "SHUTDOWN")) {
        err = CMD_SHUTDOWN;
    } else if (!strcmp(cmd, "STATS")) {
        cmd_stats(fd, server);
        err = CMD_STATS;
    } else {
        err = CMD_UNKNOWN;
    }

EXIT:
    free(cmd);
    close(fd);
    return err;
}
