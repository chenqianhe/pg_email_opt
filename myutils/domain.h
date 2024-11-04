//
// Created by Qianhe Chen on 2024/11/3.
//

#ifndef DOMAIN_H
#define DOMAIN_H

#include "postgres.h"
#include "ip.h"

/* Maximum length for a DNS label */
#define MAX_LABEL_LENGTH 63
#define MAX_DOMAIN_LENGTH 255

/*
 * Main domain validation function
 */
bool validate_email_domain(const char *domain, char **error_msg);

/*
 * Helper function to report domain validation errors using ereport
 */
void check_domain(const char *domain);

#endif //DOMAIN_H
