//
// Created by Qianhe Chen on 2024/11/3.
//

#include "local.h"
#include <string.h>

/*
 * Check if the character is a valid unquoted local-part character
 * according to RFC 5321/5322
 */
static bool
is_valid_unquoted_char(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || /* uppercase letters */
           (c >= 'a' && c <= 'z') || /* lowercase letters */
           (c >= '0' && c <= '9') || /* digits */
           c == '!' || c == '#' || c == '$' ||
           c == '%' || c == '&' || c == '\'' ||
           c == '*' || c == '+' || c == '-' ||
           c == '/' || c == '=' || c == '?' ||
           c == '^' || c == '_' || c == '`' ||
           c == '{' || c == '|' || c == '}' ||
           c == '~' || c == '.'; /* allowed special chars */
}

/*
 * Check if the character is a valid quoted local-part character
 * according to RFC 5321/5322
 */
static bool
is_valid_quoted_char(unsigned char c) {
    /* Any ASCII printable character except \ and " */
    return c >= 32 && c <= 126 && c != '\\' && c != '"';
}

/*
 * Validate email local part according to RFC 5321/5322
 * Returns true if valid, false otherwise with error details
 */
static bool
validate_email_local_part(const char *local_part, char **error_msg) {
    if (local_part == NULL) {
        *error_msg = "local part cannot be NULL";
        return false;
    }

    const size_t len = strlen(local_part);
    if (len == 0) {
        *error_msg = "local part cannot be empty";
        return false;
    }

    if (len > 64) {
        *error_msg = "local part exceeds maximum length of 64 characters";
        return false;
    }

    /* Check if the local part is quoted */
    const bool is_quoted = local_part[0] == '"' && local_part[len - 1] == '"';

    /* Handle quoted string */
    if (is_quoted) {
        bool prev_was_backslash = false;
        /* Remove quotes from length check */
        if (len <= 2) {
            *error_msg = "quoted local part cannot be empty";
            return false;
        }

        /* Check each character in quoted string */
        for (size_t i = 1; i < len - 1; i++) {
            const unsigned char c = local_part[i];

            if (prev_was_backslash) {
                /* After backslash, only certain characters are allowed */
                if (c != ' ' && c != '\t' && (c < 32 || c > 126)) {
                    *error_msg = "invalid character after backslash in quoted local part";
                    return false;
                }
                prev_was_backslash = false;
                continue;
            }

            if (c == '\\') {
                prev_was_backslash = true;
                continue;
            }

            if (!is_valid_quoted_char(c)) {
                *error_msg = "invalid character in quoted local part";
                return false;
            }
        }
    }
    /* Handle unquoted string */
    else {
        bool prev_was_dot = false;
        /* Check first and last character aren't dots */
        if (local_part[0] == '.' || local_part[len - 1] == '.') {
            *error_msg = "unquoted local part cannot begin or end with a dot";
            return false;
        }

        /* Check each character */
        for (size_t i = 0; i < len; i++) {
            const unsigned char c = local_part[i];

            /* Check for consecutive dots */
            if (c == '.') {
                if (prev_was_dot) {
                    *error_msg = "unquoted local part cannot contain consecutive dots";
                    return false;
                }
                prev_was_dot = true;
                continue;
            }
            prev_was_dot = false;

            /* Check if character is valid */
            if (!is_valid_unquoted_char(c)) {
                *error_msg = "invalid character in unquoted local part";
                return false;
            }
        }
    }

    return true;
}

/*
 * Helper function to report local part validation errors using ereport
 */
void
check_local_part(const char *local_part) {
    char *error_msg = NULL;

    if (!validate_email_local_part(local_part, &error_msg)) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("invalid local-part of email address: %s", error_msg),
                    errdetail("Local-part was: \"%s\"", local_part),
                    errhint("The local-part of an email address must follow RFC 5321/5322 rules")));
    }
}

/*
 * Helper function to check if the content of a quoted local part
 * would be valid as an unquoted local part
 */
bool
quoted_content_valid_as_unquoted(const char *quoted_part, size_t len) {
    bool prev_was_dot = false;

    /* Remove surrounding quotes */
    const char *content = quoted_part + 1;
    const size_t content_len = len - 2;

    /* Check if content would be valid unquoted */
    if (content[0] == '.' || content[content_len - 1] == '.')
        return false;

    for (size_t i = 0; i < content_len; i++) {
        const unsigned char c = content[i];

        /* Check for consecutive dots */
        if (c == '.') {
            if (prev_was_dot)
                return false;
            prev_was_dot = true;
            continue;
        }
        prev_was_dot = false;

        /* Check if character would be valid in unquoted form */
        if (!is_valid_unquoted_char(c))
            return false;
    }

    return true;
}

/*
 * Helper function to hash a local part consistently
 * whether it's quoted or unquoted
 */
uint32
hash_local_part(const char *local, size_t len) {
    /* DJB2 */
    uint32 hash = 5381;
    const bool is_quoted = local[0] == '"' && local[len - 1] == '"';
    size_t i;

    if (is_quoted && quoted_content_valid_as_unquoted(local, len)) {
        /* If quoted content is valid as unquoted, hash the unquoted form */
        for (i = 1; i < len - 1; i++)
            hash = (hash << 5) + hash + pg_tolower(local[i]);
    } else if (is_quoted) {
        /* For quoted parts that must remain quoted, include quotes in hash */
        for (i = 0; i < len; i++)
            hash = (hash << 5) + hash + local[i];
    } else {
        /* For unquoted parts, use case-insensitive hash */
        for (i = 0; i < len; i++)
            hash = (hash << 5) + hash + pg_tolower(local[i]);
    }

    return hash;
}

/*
 * Compare local parts considering quoting rules
 * Returns -1 if local1 < local2, 0 if equal, 1 if local1 > local2
 */
int
compare_local_parts(const char *local1, size_t len1,
                    const char *local2, size_t len2) {
    /* Check if parts are quoted */
    const bool is_quoted1 = len1 >= 2 && local1[0] == '"' && local1[len1 - 1] == '"';
    const bool is_quoted2 = len2 >= 2 && local2[0] == '"' && local2[len2 - 1] == '"';

    if (!is_quoted1 && !is_quoted2) {
        /* Both unquoted - do case-insensitive comparison */
        return bounded_strcasecmp(local1, len1, local2, len2);
    }
    if (is_quoted1 && is_quoted2) {
        /* Both quoted - compare content exactly */
        const size_t min_len = Min(len1, len2);
        const int cmp = memcmp(local1, local2, min_len);
        if (cmp == 0) {
            if (len1 < len2)
                return -1;
            if (len1 > len2)
                return 1;
            return 0;
        }
        return cmp;
    }
    /* One quoted, one unquoted */
    const char *quoted = is_quoted1 ? local1 : local2;
    const char *unquoted = is_quoted1 ? local2 : local1;
    const size_t quoted_len = is_quoted1 ? len1 : len2;
    const size_t unquoted_len = is_quoted1 ? len2 : len1;

    /* Check if quoted content would be valid as unquoted */
    if (quoted_content_valid_as_unquoted(quoted, quoted_len)) {
        /* Compare unquoted form with quoted content */
        const int cmp = bounded_strcasecmp(quoted + 1, quoted_len - 2,
                                           unquoted, unquoted_len);
        /* Reverse comparison if second string was quoted */
        return is_quoted1 ? -cmp : cmp;
    }
    /*
     * If quoted content can't be unquoted, treat quoted form
     * as greater (comes after) unquoted form
     */
    return is_quoted1 ? 1 : -1;
}
