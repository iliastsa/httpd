#ifndef HTTP_IO_H
#define HTTP_IO_H

#include <stdio.h>

int read_bytes(int fd, char *buf, size_t n_bytes);
int write_bytes(int fd, char *buf, size_t n_bytes);
int write_file(int fd, char *filepath);
#endif
