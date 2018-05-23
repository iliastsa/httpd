#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "server_types.h"
#include "network_io.h"

#define CMD_SHUTDOWN 10
#define CMD_STATS     9
#define CMD_UNKNOWN   8

int accept_command(int fd, ServerResources *server);

#endif
