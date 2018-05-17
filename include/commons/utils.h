#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

#if defined(DEBUG)
    #define P_DEBUG(...) fprintf(stderr,"[DEBUG] : "__VA_ARGS__)
#else
    #define P_DEBUG(...)
#endif

#endif
