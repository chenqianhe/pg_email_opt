/*
 * Created by Qianhe Chen on 2024/11/3.
 */

#include "ip.h"
#include <string.h>

static bool
is_valid_ipv4(const char *ipv4_str)
{
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
            /* Check for consecutive dots or dot at start */
            if (p == ipv4_str || *(p-1) == '.')
                return false;

            dots++;
            curr_num = 0;
            continue;
        }

        /* Only digits are allowed */
        if (*p < '0' || *p > '9')
            return false;

        curr_num = curr_num * 10 + (*p - '0');
        if (curr_num > 255)
            return false;
    }

    /* Check final position and total dots */
    return *(p-1) != '.' && dots == 3;
}

static bool
is_valid_ipv6_segment(const char *seg, int len)
{
    /* Segment length check */
    if (len > 4 || len == 0)
        return false;

    /* Check each character is valid hex */
    for (int i = 0; i < len; i++)
    {
        const char c = seg[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
            return false;
    }

    return true;
}

static bool
is_valid_ipv6(const char *ipv6_str)
{
    int seg_len;
    int segments = 0;
    bool has_double_colon = false;

    /* Empty string is invalid */
    if (!ipv6_str || *ipv6_str == '\0')
        return false;

    const char *p = ipv6_str;
    const char *seg_start = p;

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

bool
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