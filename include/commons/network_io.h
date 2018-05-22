#ifndef NETWORK_IO_H
#define NETWORK_IO_H

#include <stdio.h>

#define IO_TIMEOUT -2
#define IO_UNEXPECTED -1
#define IO_OK 0

int read_bytes(int fd, char *buf, int timeout, size_t n_bytes);
int write_bytes(int fd, char *buf, int timeout, size_t n_bytes);
int write_file(int fd, char *filepath, int timeout);

#endif
