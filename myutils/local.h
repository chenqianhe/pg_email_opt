//
// Created by Qianhe Chen on 2024/11/3.
//

#ifndef LOCAL_H
#define LOCAL_H

#include "postgres.h"
#include "common.h"

/* Function declarations */

/*
 * Validate email local part according to RFC 5321/5322
 * Returns true if valid, false otherwise with error details
 */
static bool validate_email_local_part(const char *local_part, char **error_msg);

/*
 * Helper function to report local part validation errors using ereport
 */
void check_local_part(const char *local_part);

/*
 * Helper function to check if the content of a quoted local part
 * would be valid as an unquoted local part
 */
bool quoted_content_valid_as_unquoted(const char *quoted_part, size_t len);

/*
 * Helper function to hash a local part consistently
 * whether it's quoted or unquoted
 */
uint32 hash_local_part(const char *local, size_t len);

/*
 * Compare local parts considering quoting rules
 * Returns -1 if local1 < local2, 0 if equal, 1 if local1 > local2
 */
int compare_local_parts(const char *local1, size_t len1,
                             const char *local2, size_t len2);

#endif //LOCAL_H
