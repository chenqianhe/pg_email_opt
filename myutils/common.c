//
// Created by Qianhe Chen on 2024/11/4.
//

#include "common.h"

/*
 * Helper function to compare two strings case-insensitively,
 * with length limits for each string
 */
int
bounded_strcasecmp(const char *s1, const size_t len1,
                   const char *s2, const size_t len2) {
    const size_t min_len = Min(len1, len2);
    const int cmp = pg_strncasecmp(s1, s2, min_len);
    if (cmp == 0) {
        /* If common prefix matches, longer string is greater */
        if (len1 < len2)
            return -1;
        if (len1 > len2)
            return 1;
        return 0;
    }
    return cmp;
}
