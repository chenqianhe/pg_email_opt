//
// Created by Qianhe Chen on 2024/11/3.
//

#ifndef DOMAIN_H
#define DOMAIN_H

#include "postgres.h"
#include "utils/builtins.h"
#include "utils/inet.h"    /* for IP address validation */

#include "ip.h"

/* Maximum length for a DNS label */
#define MAX_LABEL_LENGTH 63
#define MAX_DOMAIN_LENGTH 255

/*
 * Check if character is valid in a domain name (LDH rule)
 */
static bool
is_valid_domain_char(const unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ||    /* uppercase letters */
           (c >= 'a' && c <= 'z') ||    /* lowercase letters */
           (c >= '0' && c <= '9') ||    /* digits */
           c == '-' ||                  /* hyphen */
           c == '.';                    /* dot separator */
}

/*
 * Check if string is all numeric
 */
static bool
is_all_numeric(const char *str)
{
    while (*str)
    {
        if (!(*str >= '0' && *str <= '9'))
            return false;
        str++;
    }
    return true;
}

/*
 * Validate a standard domain name
 */
static bool
validate_standard_domain(const char *domain, char **error_msg)
{
    const char *p;
    bool had_dot = false;

    /* Check total length */
    if (strlen(domain) > MAX_DOMAIN_LENGTH)
    {
        *error_msg = "domain name exceeds maximum length";
        return false;
    }

    /* Validate each label */
    const char *label_start = domain;
    size_t label_len = 0;
    for (p = domain; *p; p++)
    {
        if (*p == '.')
        {
            /* Check label length */
            if (label_len == 0)
            {
                *error_msg = "empty label in domain name";
                return false;
            }
            if (label_len > MAX_LABEL_LENGTH)
            {
                *error_msg = "domain label exceeds maximum length";
                return false;
            }

            /* Check if label starts or ends with hyphen */
            if (label_start[0] == '-' || *(p-1) == '-')
            {
                *error_msg = "domain label cannot start or end with hyphen";
                return false;
            }

            /* Reset for next label */
            label_start = p + 1;
            label_len = 0;
            had_dot = true;
            continue;
        }

        /* Validate character */
        if (!is_valid_domain_char(*p))
        {
            *error_msg = "invalid character in domain name";
            return false;
        }

        label_len++;
    }

    /* Check final label */
    if (label_len == 0)
    {
        *error_msg = "empty label in domain name";
        return false;
    }
    if (label_len > MAX_LABEL_LENGTH)
    {
        *error_msg = "domain label exceeds maximum length";
        return false;
    }
    if (label_start[0] == '-' || *(p-1) == '-')
    {
        *error_msg = "domain label cannot start or end with hyphen";
        return false;
    }

    /* Must have at least one dot for a full domain */
    if (!had_dot)
    {
        *error_msg = "domain must contain at least two parts";
        return false;
    }

    /* Check if top-level domain is all numeric */
    if (is_all_numeric(label_start))
    {
        *error_msg = "top-level domain cannot be all numeric";
        return false;
    }

    return true;
}

/*
 * Main domain validation function
 */
static bool
validate_email_domain(const char *domain, char **error_msg)
{
    if (domain == NULL)
    {
        *error_msg = "domain cannot be NULL";
        return false;
    }

    if (strlen(domain) == 0)
    {
        *error_msg = "domain cannot be empty";
        return false;
    }

    /* Check if it's an IP literal */
    if (domain[0] == '[')
    {
        return validate_ip_literal(domain, error_msg);
    }

    /* Standard domain name validation */
    return validate_standard_domain(domain, error_msg);
}

/*
 * Helper function to report domain validation errors using ereport
 * https://www.wikiwand.com/en/articles/Email_address#Domain
 */
inline void
check_domain(const char *domain)
{
    char *error_msg = NULL;

    if (!validate_email_domain(domain, &error_msg))
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                 errmsg("invalid domain part of email address: %s", error_msg),
                 errdetail("Domain was: \"%s\"", domain),
                 errhint("Domain must follow DNS naming rules or be a valid IP address literal")));
    }
}

#endif //DOMAIN_H
