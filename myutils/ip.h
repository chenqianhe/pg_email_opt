//
// Created by Qianhe Chen on 2024/11/3.
//

#ifndef IP_H
#define IP_H

#include "postgres.h"

/*
 * Validate IPv4 address
 */
static bool
is_valid_ipv4(const char *ipv4_str)
{
    int segments = 0;
    int curr_num = 0;
    int dots = 0;
    const char *p;

    /* Empty string is invalid */
    if (!ipv4_str || *ipv4_str == '\0')
        return false;

    /* Parse each character */
    for (p = ipv4_str; *p; p++)
    {
        if (*p == '.')
        {
            /* Check for consecutive dots */
            if (p == ipv4_str || *(p-1) == '.')
                return false;

            /* Check segment value */
            if (curr_num > 255)
                return false;

            dots++;
            curr_num = 0;
            continue;
        }

        /* Only digits are allowed */
        if (*p < '0' || *p > '9')
            return false;

        /* Build current number */
        curr_num = curr_num * 10 + (*p - '0');

        /* Check for overflow */
        if (curr_num > 255)
            return false;
    }

    /* Last character shouldn't be a dot */
    if (*(p-1) == '.')
        return false;

    /* Check final segment */
    if (curr_num > 255)
        return false;

    /* Must have exactly 3 dots (4 segments) */
    return (dots == 3);
}

/*
 * Validate a single IPv6 segment (hexadecimal number)
 */
static bool
is_valid_ipv6_segment(const char *seg, int len)
{
    int i;

    /* Segment length check */
    if (len > 4 || len == 0)
        return false;

    /* Check each character is valid hex */
    for (i = 0; i < len; i++)
    {
        char c = seg[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
            return false;
    }

    return true;
}

/*
 * Validate IPv6 address
 */
static bool
is_valid_ipv6(const char *ipv6_str)
{
    const char *p;
    const char *seg_start;
    int seg_len;
    int segments = 0;
    bool has_double_colon = false;
    int seg_before_double_colon = 0;

    /* Empty string is invalid */
    if (!ipv6_str || *ipv6_str == '\0')
        return false;

    p = ipv6_str;
    seg_start = p;

    /* Skip "IPv6:" prefix if present */
    if (strncmp(p, "IPv6:", 5) == 0)
        p += 5;

    /* Process each character */
    while (*p)
    {
        if (*p == ':')
        {
            /* Check segment before colon */
            seg_len = p - seg_start;
            if (seg_len > 0)
            {
                if (!is_valid_ipv6_segment(seg_start, seg_len))
                    return false;
                segments++;
            }

            /* Handle double colon */
            if (*(p + 1) == ':')
            {
                if (has_double_colon)  /* Only one :: allowed */
                    return false;
                has_double_colon = true;
                seg_before_double_colon = segments;
                p++;
            }
            else if (p == ipv6_str || *(p - 1) == ':')  /* No empty segments unless :: */
                return false;

            seg_start = p + 1;
        }
        p++;
    }

    /* Process last segment */
    seg_len = p - seg_start;
    if (seg_len > 0)
    {
        if (!is_valid_ipv6_segment(seg_start, seg_len))
            return false;
        segments++;
    }

    /* Check total number of segments */
    if (has_double_colon)
    {
        /* With ::, we can have fewer segments as :: expands to multiple 0s */
        if (segments > 7)
            return false;
    }
    else
    {
        /* Without ::, we must have exactly 8 segments */
        if (segments != 8)
            return false;
    }

    return true;
}

/*
 * Validate an IP literal address
 * Supports both IPv4 and IPv6
 */
inline bool
validate_ip_literal(const char *ip_str, char **error_msg)
{
    /* Remove brackets */
    const int len = strlen(ip_str);
    if (len < 2 || ip_str[0] != '[' || ip_str[len-1] != ']')
    {
        *error_msg = "IP literal must be enclosed in square brackets";
        return false;
    }

    /* Extract IP address */
    char *ip = pnstrdup(ip_str + 1, len - 2);
    bool result;

    /* Check for IPv6 prefix */
    if (strncmp(ip, "IPv6:", 5) == 0)
    {
        /* Validate IPv6 address */
        result = is_valid_ipv6(ip);
        if (!result)
            *error_msg = "invalid IPv6 address";
    }
    else
    {
        /* Validate IPv4 address */
        result = is_valid_ipv4(ip);
        if (!result)
            *error_msg = "invalid IPv4 address";
    }

    pfree(ip);
    return result;
}

#endif //IP_H
