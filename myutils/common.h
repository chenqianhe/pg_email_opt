//
// Created by Qianhe Chen on 2024/11/4.
//

#ifndef COMMON_H
#define COMMON_H

#include <c.h>
#include <port.h>

/*
 * Helper function to compare two strings case-insensitively,
 * with length limits for each string
 */
int bounded_strcasecmp(const char *s1, const size_t len1,
                       const char *s2, const size_t len2);

#endif //COMMON_H
