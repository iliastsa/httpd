#ifndef UTILS_H
#define UTILS_H

#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(DEBUG)
    #define P_DEBUG(...) fprintf(stderr,"[DEBUG] : "__VA_ARGS__)
#else
    #define P_DEBUG(...)
#endif

#define ERR(msg) fprintf(stderr, "%s\n", msg)
#define P_ERR(msg,err) fprintf(stderr, "%s : %s\n", msg, strerror(err))

typedef struct {
    unsigned long long int milisec;
    unsigned long long int seconds;
    unsigned long long int minutes;
    unsigned long long int hours;
} TimeFormat;

int copy_until_delim(char *source, char *dest, int sz, int *offset, const char *delim);
char count_bytes(char *filepath, long *n_bytes);
void diff_time(struct timeval *t_start, struct timeval *t_end, TimeFormat *t_diff);
int check_dir_access(char *dir);

#endif
