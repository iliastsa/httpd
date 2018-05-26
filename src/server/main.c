#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "server_manager.h"

void print_usage(){
    fprintf(stderr, "Usage : ./myhttpd -p <http_port> -c <cmd_port> -t <num_threads> -d <root_dir>\n");
}

void print_repeat_error(char p){
    fprintf(stderr, "Error : parameter -%c passed multiple times.\n", p);
}

int main(int argc, char *argv[]) {
    if (argc != 9) {
        print_usage();
        return -1;
    }

    int p;
    int c;
    int t;
    char *d;

    char read_p = 0;
    char read_c = 0;
    char read_t = 0;
    char read_d = 0;

    // Parse arguments
    int option;
    char *end;
    while ((option = getopt(argc, argv, "p:c:t:d:")) != -1){
        switch (option){
            case 'p':
                if (read_p){
                    print_repeat_error('p');
                    print_usage();
                    return -1;
                }

                p = strtol(optarg, &end, 10);

                if (*end != '\0' || p <= 0){
                    fprintf(stderr, "Error : -p argument must be a positive integer.\n");
                    return -1;
                }

                read_p = 1;
                break;

            case 'c':
                // If we already read option d, print error message
                if (read_c){
                    print_repeat_error('c');
                    print_usage();
                    return -1;
                }

                c = strtol(optarg, &end, 10);

                if (*end != '\0' || c <= 0){
                    fprintf(stderr, "Error : -c argument must be a positive integer.\n");
                    return -1;
                }

                read_c = 1;
                break;

            case 't':
                // If we already read option d, print error message
                if (read_t){
                    print_repeat_error('t');
                    print_usage();
                    return -1;
                }

                t = strtol(optarg, &end, 10);

                if (*end != '\0' || t <= 0){
                    fprintf(stderr, "Error : -t argument must be a positive integer.\n");
                    return -1;
                }

                read_t = 1;
                break;

            case 'd':
                // If we already read option d, print error message
                if (read_d){
                    print_repeat_error('d');
                    print_usage();
                    return -1;
                }

                d = optarg;

                read_d = 1;
                break;

            case '?':
                print_usage();
                return -2;

            default:
                return -3;
        }
    }

    // Argument parsing was sucessful
    ServerResources *server = server_create(p, c, t, d);

    if (server == NULL)
        return -1;

    if (server_init_sockets(server, 10) < 0)
        return -1;
    else
        server_run(server);

    free_server(server);

    return 0;
}
