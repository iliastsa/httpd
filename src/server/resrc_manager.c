#include "server_types.h"
#include "server_types.h"

ServerResources *server_create(int s_port, int c_port, int n_threads, char *r_dir) {
    ServerResources *server = (ServerResources*) malloc(sizeof(ServerResources));

    if (server == NULL)
}
