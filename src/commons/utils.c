#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "utils.h"

int copy_until_delim(char *source, char *dest, int sz, int *offset, const char *delim){
    int pattern_end = strlen(delim);

    for (int i = 0; i < sz; ++i) {
        dest[i] = source[i];

        if (source[i] == delim[*offset]) {
            //P_DEBUG("Found symbol num %d from end of header delimiter\n", *offset);
            (*offset)++;
        }
        else
            *offset = 0;

        if (*offset == pattern_end)
            return i;
    }

    return -1;
}

char count_bytes(char *filepath, long *n_bytes) {
    FILE *fp = fopen(filepath, "r");

    if (fp == NULL)
        return -1;

    if(fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }

    long sz = ftell(fp);

    if (sz < 0) {
        fclose(fp);
        return -1;
    }

    *n_bytes = sz;

    fclose(fp);
    return 0;
}
