#include <sys/time.h>
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

void diff_time(struct timeval *t_start, struct timeval *t_end, TimeFormat *t_diff) {
    unsigned long long int s_diff = t_end->tv_sec - t_start->tv_sec;
    unsigned long long int ms_diff = s_diff * 1000 +  (t_end->tv_usec - t_start->tv_usec) / 1000;

    t_diff->hours = ms_diff / 3600000;

    ms_diff %= 3600000;

    t_diff->minutes = ms_diff / 60000;

    ms_diff %= 60000;

    t_diff->seconds = ms_diff / 1000;

    t_diff->milisec = ms_diff % 1000;
}
