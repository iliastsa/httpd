#include <string.h>

#include "parse_utils.h"

char check_line_termination(char *header) {
    int len = strlen(header);

    // Iterate over all characters in the header
    for (int i = 0; i < len; ++i)
        // Catch any unpaired \r or \n in the header
        if ( (header[i] == '\r' && header[i+1] != '\n') ||
             (i == 0 && header[i] == '\n') ||
             (i  > 0 && header[i] == '\n' && header[i-1] != '\r')
           )
            return -1;

    return 0;
}
