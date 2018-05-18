#ifndef SERVER_MANAGER_H
#define SERVER_MANAGER_H

#include "server_types.h"

ServerResources *server_create(int s_port, int c_port, int n_threads, char *r_dir);
char server_run(ServerResources *server);
char server_init_sockets(ServerResources *server, int backlog);
#endif
