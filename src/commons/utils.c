#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "utils.h"

char count_bytes(char *filepath, long *n_bytes) {
    FILE *fp = fopen(filepath, "r");

    if (fp == NULL)
        return -1;

    if(fseek(fp, 0L, SEEK_END) != 0)
        return -1;

    long sz = ftell(fp);

    if (sz < 0)
        return -1;

    rewind(fp);

    *n_bytes = sz;
    return 0;
}
