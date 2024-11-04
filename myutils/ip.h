/*
* Created by Qianhe Chen on 2024/11/3.
 */

#ifndef IP_H
#define IP_H

#include "postgres.h"

/*
 * Validate a single IPv6 segment (hexadecimal number)
 */
static bool is_valid_ipv6_segment(const char *seg, int len);

/*
 * Validate IPv4 address
 */
static bool is_valid_ipv4(const char *ipv4_str);

/*
 * Validate IPv6 address
 */
static bool is_valid_ipv6(const char *ipv6_str);

/*
 * Validate an IP literal address
 * Supports both IPv4 and IPv6
 */
bool validate_ip_literal(const char *ip_str, char **error_msg);

#endif //IP_H
