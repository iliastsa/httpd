#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <ctype.h>
#include <string.h>

#include "network_io.h"
#include "utils.h"

/*
 * Copies characters from the source buffer into dest, until the delimiter
 * is found, or we have reached the end of the buffer. This function is
 * reentrant, so it can support the above functionality even if the buffer
 * is given in chunks.
 *
 * Params:
 * - char *source      : The source buffer.
 * - char *dest        : The destination buffer.
 * - int sz            : The size of the source buffer.
 * - int *offset       : The offset used for reentering the function, so we can
 *                       keep track in between different calls.
 * - const char *delim : The delimiter we are looking for.
 *
 * Returns:
 * - The index i, where the end of the delimiter was found.
 * - -1 if the delimiter was not found.
 */
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

/*
 * Checks if a directory is empty.
 *
 * Params:
 * - char *dir_path : The path of the directory in question.
 *
 * Returns:
 * -  1 if the directory is empty.
 * -  0 if it is not empty.
 * - -1 if an error occured.
 */
char is_dir_empty(const char *dir_path) {
    DIR *dir = opendir(dir_path);

    if (dir == NULL)
        return -1;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        else
            break;
    }

    closedir(dir);

    return entry == NULL;
}

/*
 * Counts the bytes in the given file.
 *
 * Params:
 * - char *filepath : The path of the file we are interested in.
 * - long *n_bytes  : Pointer to the variable where the result will
 *                    be stored.
 *
 * Returns:
 * -  0 if no error occurred.
 * - -1 otherwise.
 */
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

/*
 * Calculates the time difference in hours, seconds and milliseconds
 * between two timeval structs.
 *
 * Params:
 * - struct timeval *t_start : The timeval struct for the start time.
 * - struct timeval *t_end   : The timeval struct for the end time.
 * - struct timeval *t_diff  : The timeval struct where the result will be
 *                             stored.
 *
 * Returns: -
 */
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

/*
 * Checks if a directory exists and the user has read and 
 * write access.
 *
 * Params:
 * - char *dir : The path of the directory in question.
 *
 * Returns:
 * -  0 if the above hold true.
 * - -1 otherwise.
 */
int check_dir_access(char *dir) {
    struct stat d_stats;

    if (stat(dir, &d_stats) < 0)
        return -1;

    if (!S_ISDIR(d_stats.st_mode))
        return -1;

    if (!(d_stats.st_mode & S_IRUSR))
        return -1;

    return 0;
}

/*
 * Converts a string to lowercase.
 *
 * Params:
 * - char *buf : The string to be convterted to lowercase.
 *
 * Returns: -
 */
void str_to_lowercase(char *str) {
    size_t len = strlen(str);

    for (size_t i = 0; i < len; ++i)
        str[i] = tolower(str[i]);
}

/*
 * Creates a file and writes the contents of the specified
 * buffer in it.
 *
 * Params:
 * - char *buf      : The buffer we want to write to the file.
 * - char *filepath : The path of the file we want to create.
 * - size_t len     : The number of bytes in the buffer.
 *
 * Returns:
 * - IO_OK, if no error occured.
 * - An appropriate io error code, if an error occurred.
 */
int write_to_file(char *buf, char *filepath, size_t len) {
    int file = open(filepath, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (file < 0) {
        P_ERR("Error during open", errno);
        return IO_UNEXPECTED;
    }

    int status = write_bytes(file, buf, -1, len);

    close(file);

    return status;
}

// Simple ceiling division.
long ceil_division(long a, long b){
    return (a%b == 0) ? a/b : a/b + 1;
}
