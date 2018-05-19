#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined(DEBUG)
    #define P_DEBUG(...) fprintf(stderr,"[DEBUG] : "__VA_ARGS__)
#else
    #define P_DEBUG(...)
#endif

#define ERR(msg) fprintf(stderr, "%s\n", msg)
#define P_ERR(msg,err) fprintf(stderr, "%s : %s\n", msg, strerror(err))

char count_bytes(char *filepath, long *n_bytes);
#endif
