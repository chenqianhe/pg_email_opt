//
// Created by Qianhe Chen on 2024/11/3.
//

#include "postgres.h"

#include "access/htup_details.h"
#include "utils/builtins.h"
#include "fmgr.h"
#include "utils/palloc.h"

#include "myutils/local.h"
#include "myutils/domain.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

/*
 * Structure for representing an email address in PostgreSQL.
 * Format: local-part@domain
 * This is designed to efficiently store and manipulate email addresses
 * while maintaining PostgreSQL's TOAST-able varlena format.
 */
typedef struct {
    /* varlena header for storing total struct length */
    char vl_len_[4];

    /* offset to where domain starts in data array */
    uint16 domain_offset;

    /* lengths of local-part and domain */
    uint16 local_len;
    uint16 domain_len;

    /* actual data storage: [local_part]\0[domain]\0
     * both parts are null-terminated for easy string handling
     */
    char data[FLEXIBLE_ARRAY_MEMBER];
} EMAIL_ADDR;

/*
 * Macros for working with email_addr type
 */
#define PG_GETARG_EMAIL_ADDR_P(n)     ((EMAIL_ADDR *) PG_GETARG_POINTER(n))
#define PG_GETARG_EMAIL_ADDR_PP(n)  ((EMAIL_ADDR *) PG_DETOAST_DATUM(PG_GETARG_DATUM(n)))
#define PG_GETARG_EMAIL_ADDR_COPY(n) ((EMAIL_ADDR *) PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(n)))

/* For returning email_addr values */
#define PG_RETURN_EMAIL_ADDR(x)    PG_RETURN_POINTER(x)

/*
 * Creates a new EMAIL_ADDR structure from local-part and domain.
 *
 * Arguments:
 *     local_part - the part before @ in email
 *     domain - the part after @ in email
 *
 * Returns:
 *     Pointer to newly allocated EMAIL_ADDR structure
 *
 * Note: The caller must ensure input strings are valid.
 */
EMAIL_ADDR *
make_email_addr(const char *local_part, const char *domain) {
    const Size local_len = strlen(local_part);
    const Size domain_len = strlen(domain);

    /* Validate lengths */
    if (local_len > 64)
        ereport(ERROR,
            (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
                errmsg("email local part too long"),
                errdetail("Maximum length is 64 characters.")));

    if (domain_len > 255)
        ereport(ERROR,
            (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
                errmsg("email domain too long"),
                errdetail("Maximum length is 255 characters.")));

    /* Calculate total size with proper alignment */
    const Size total_size = VARHDRSZ +
                            offsetof(EMAIL_ADDR, data) +
                            local_len + 1 + /* local part + null terminator */
                            domain_len + 1; /* domain + null terminator */

    /* Allocate and zero memory */
    EMAIL_ADDR *result = (EMAIL_ADDR *) palloc0(total_size);

    /* Set varlena header */
    SET_VARSIZE(result, total_size);

    /* Set lengths */
    result->local_len = local_len;
    result->domain_len = domain_len;
    result->domain_offset = local_len + 1; /* Position after local part's null terminator */

    /* Copy local part */
    memcpy(result->data, local_part, local_len);
    result->data[local_len] = '\0';

    /* Copy domain */
    memcpy(result->data + result->domain_offset, domain, domain_len);
    result->data[result->domain_offset + domain_len] = '\0';

    return result;
}

/*
 * Returns pointer to local-part of email address.
 * Result is null-terminated string.
 */
const char *
get_local_part(const EMAIL_ADDR *addr) {
    Assert(addr != NULL);
    return addr->data;
}

/*
 * Returns pointer to domain part of email address.
 * Result is null-terminated string.
 */
const char *
get_domain(const EMAIL_ADDR *addr) {
    Assert(addr != NULL);
    return addr->data + addr->domain_offset;
}

/*
 * Returns newly allocated string containing complete email address
 * in "local-part@domain" format.
 * Caller is responsible for pfree'ing the result.
 */
char *
get_full_email(const EMAIL_ADDR *addr) {
    Assert(addr != NULL);

    /* Allocate memory for: local-part + @ + domain + null terminator */
    char *result = palloc(addr->local_len + 1 + addr->domain_len + 1);

    /* Construct full address */
    memcpy(result, addr->data, addr->local_len);
    result[addr->local_len] = '@';
    memcpy(result + addr->local_len + 1,
           addr->data + addr->domain_offset,
           addr->domain_len);
    result[addr->local_len + 1 + addr->domain_len] = '\0';

    return result;
}

/*
 * Compare two email addresses for equality, considering RFC 5321/5322 rules
 * Returns true if addresses are equal, false otherwise
 */
bool
email_addr_equals(const EMAIL_ADDR *addr1, const EMAIL_ADDR *addr2) {
    /* Check for NULL inputs */
    if (!addr1 || !addr2)
        return false;

    /* First compare domains (case-insensitive) */
    int cmp = bounded_strcasecmp(get_domain(addr1), addr1->domain_len,
                                 get_domain(addr2), addr2->domain_len);
    if (cmp != 0)
        return false;;

    /* If domains are equal, compare local parts */
    cmp = compare_local_parts(get_local_part(addr1), addr1->local_len,
                              get_local_part(addr2), addr2->local_len);
    if (cmp != 0)
        return false;

    return true;
}

/*
 * Hash function for email addresses that produces the same hash
 * for equivalent addresses according to RFC 5321/5322 rules
 */
uint32
email_addr_hash(const EMAIL_ADDR *addr) {
    /* Handle NULL input */
    if (!addr)
        return 0;

    /* Hash local part */
    uint32 hash = hash_local_part(get_local_part(addr), addr->local_len);

    /* Add @ separator to hash */
    hash = (hash << 5) + hash + '@';

    /* Hash domain (always case-insensitive) */
    for (int i = 0; i < addr->domain_len; i++)
        hash = (hash << 5) + hash +
               pg_tolower(get_domain(addr)[i]);

    return hash;
}

/*
 * Find the rightmost @ character that is not within quotes.
 * Returns NULL if no valid @ character is found.
 * Handles escaped quotes and validates quote structure.
 */
static const char *
find_valid_at(const char *input_text) {
    const char *at_pos = NULL;
    const char *p = input_text;
    bool in_quotes = false;
    bool escaped = false;

    /* Scan the entire string */
    while (*p != '\0') {
        if (escaped) {
            /* Previous character was backslash - this character is escaped */
            escaped = false;
        } else if (*p == '\\') {
            /* Start of escape sequence */
            escaped = true;
        } else if (*p == '"') {
            /* Toggle quote state */
            in_quotes = !in_quotes;
        } else if (*p == '@' && !in_quotes) {
            /* Found an @ outside of quotes */
            at_pos = p;
        }
        p++;
    }

    /* Check for unterminated quotes */
    if (in_quotes) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("unterminated quotes in email address: \"%s\"",
                        input_text)));
    }

    /* Check for trailing backslash */
    if (escaped) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("invalid trailing backslash in email address: \"%s\"",
                        input_text)));
    }

    return at_pos;
}

/*
 * Input function: text representation to internal format
 */
PG_FUNCTION_INFO_V1(email_addr_in);

Datum
email_addr_in(PG_FUNCTION_ARGS) {
    char *input_text = PG_GETARG_CSTRING(0);

    /* Find the @ character */
    const char *at_pos = find_valid_at(input_text);
    if (at_pos == NULL)
        ereport(ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                errmsg("invalid input syntax for type emailaddr: \"%s\"",
                    input_text)));

    /* Split the address and create a new email addr */
    char *local_part = pnstrdup(input_text, at_pos - input_text);
    char *domain = pstrdup(at_pos + 1);

    check_local_part(local_part);
    check_domain(domain);

    const EMAIL_ADDR *result = make_email_addr(local_part, domain);

    pfree(local_part);
    pfree(domain);

    PG_RETURN_POINTER(result);
}

/*
 * Output function for EMAIL_ADDR type
 * Converts internal representation to standard email string format
 */
PG_FUNCTION_INFO_V1(email_addr_out);

Datum
email_addr_out(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    if (email == NULL)
        elog(ERROR, "null email address");

    /* Debug: Print structure details */
    elog(NOTICE, "Structure details:");
    elog(NOTICE, "  varlena size: %d", VARSIZE(email));
    elog(NOTICE, "  domain_offset: %d", email->domain_offset);
    elog(NOTICE, "  local_len: %d", email->local_len);
    elog(NOTICE, "  domain_len: %d", email->domain_len);

    /* Debug: Print raw data */
    elog(NOTICE, "Raw data (hex):");
    for (int i = 0; i < VARSIZE(email); i++) {
        elog(NOTICE, "  byte[%d]: %02x", i, ((unsigned char *)email)[i]);
    }

    /* Get parts with bounds checking */
    const char *local_part = get_local_part(email);
    elog(NOTICE, "Local part pointer: %p", (void*)local_part);
    elog(NOTICE, "Local part content: '%.*s'", email->local_len, local_part);

    const char *domain = get_domain(email);
    elog(NOTICE, "Domain pointer: %p", (void*)domain);
    elog(NOTICE, "Domain content: '%.*s'", email->domain_len, domain);

    /* Calculate total length needed */
    const int total_len = email->local_len + 1 + email->domain_len + 1;

    /* Allocate and construct result */
    char *result = palloc(total_len);

    /* Copy with explicit lengths */
    memcpy(result, local_part, email->local_len);
    result[email->local_len] = '@';
    memcpy(result + email->local_len + 1, domain, email->domain_len);
    result[total_len - 1] = '\0';

    /* Debug: Print final result */
    elog(NOTICE, "Final result: '%s'", result);

    PG_RETURN_CSTRING(result);
}

/*
 * Compare two email addresses
 * Returns -1 if addr1 < addr2, 0 if equal, 1 if addr1 > addr2
 */
PG_FUNCTION_INFO_V1(email_addr_cmp);

Datum
email_addr_cmp(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *addr1 = PG_GETARG_EMAIL_ADDR_PP(0);
    const EMAIL_ADDR *addr2 = PG_GETARG_EMAIL_ADDR_PP(1);

    /* Handle NULLs */
    if (addr1 == NULL) {
        if (addr2 == NULL)
            PG_RETURN_INT32(0); /* NULL == NULL */
        PG_RETURN_INT32(-1); /* NULL < non-NULL */
    }
    if (addr2 == NULL)
        PG_RETURN_INT32(1); /* non-NULL > NULL */

    /* First compare domains (case-insensitive) */
    int cmp = bounded_strcasecmp(get_domain(addr1), addr1->domain_len,
                                 get_domain(addr2), addr2->domain_len);
    if (cmp != 0)
        PG_RETURN_INT32(cmp);

    /* If domains are equal, compare local parts */
    cmp = compare_local_parts(get_local_part(addr1), addr1->local_len,
                              get_local_part(addr2), addr2->local_len);

    PG_RETURN_INT32(cmp);
}


PG_FUNCTION_INFO_V1(email_addr_lt);

Datum
email_addr_lt(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(email_addr_le);

Datum
email_addr_le(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(email_addr_gt);

Datum
email_addr_gt(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(email_addr_ge);

Datum
email_addr_ge(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(email_addr_eq);

Datum
email_addr_eq(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(email_addr_ne);

Datum
email_addr_ne(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp != 0);
}

/*
 * Hash function for email addresses to support hash indexes
 */
PG_FUNCTION_INFO_V1(email_hash);

Datum
email_hash(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);
    uint32 hash = email_addr_hash(email);

    /* PostgreSQL hash indexes reserve 0 and 0xFFFFFFFF */
    if (hash == 0 || hash == 0xFFFFFFFF)
        hash = 0x12345678;

    PG_RETURN_UINT32(hash);
}

/*
 * Extract the local part from an email address
 */
PG_FUNCTION_INFO_V1(email_addr_get_local_part);

Datum
email_addr_get_local_part(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    /* Handle NULL input */
    if (email == NULL)
        PG_RETURN_NULL();

    /* Debug log */
    elog(DEBUG1, "get_local_part: local_len=%d", email->local_len);

    /* Allocate result text */
    text *result = (text *) palloc(VARHDRSZ + email->local_len);
    SET_VARSIZE(result, VARHDRSZ + email->local_len);

    /* Copy data with explicit length */
    const char *local_part = get_local_part(email);
    memcpy(VARDATA(result), local_part, email->local_len);

    PG_RETURN_TEXT_P(result);
}

/*
 * Extract the domain part from an email address
 */
PG_FUNCTION_INFO_V1(email_addr_get_domain);

Datum
email_addr_get_domain(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    /* Handle NULL input */
    if (email == NULL)
        PG_RETURN_NULL();

    /* Debug log */
    elog(DEBUG1, "get_domain: domain_len=%d", email->domain_len);

    /* Allocate result text */
    text *result = (text *) palloc(VARHDRSZ + email->domain_len);
    SET_VARSIZE(result, VARHDRSZ + email->domain_len);

    /* Copy data with explicit length */
    const char *domain = get_domain(email);
    memcpy(VARDATA(result), domain, email->domain_len);

    PG_RETURN_TEXT_P(result);
}

/*
 * Get normalized local part (unquoted form if possible)
 */
PG_FUNCTION_INFO_V1(email_addr_normalized_local_part);

Datum
email_addr_normalized_local_part(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);
    text *result;
    size_t result_len;

    /* Handle NULL input */
    if (email == NULL)
        PG_RETURN_NULL();

    const char *local_part = get_local_part(email);
    const bool is_quoted = local_part[0] == '"' && local_part[email->local_len - 1] == '"';

    /* If quoted and can be unquoted, return unquoted form */
    if (is_quoted && quoted_content_valid_as_unquoted(local_part, email->local_len)) {
        result_len = email->local_len - 2; /* remove quotes */
        result = (text *) palloc(VARHDRSZ + result_len);
        SET_VARSIZE(result, VARHDRSZ + result_len);
        memcpy(VARDATA(result), local_part + 1, result_len);
    } else {
        /* Return as-is */
        result_len = email->local_len;
        result = (text *) palloc(VARHDRSZ + result_len);
        SET_VARSIZE(result, VARHDRSZ + result_len);
        memcpy(VARDATA(result), local_part, result_len);
    }

    PG_RETURN_TEXT_P(result);
}

/*
 * Get normalized domain (always lowercase)
 */
PG_FUNCTION_INFO_V1(email_addr_normalized_domain);

Datum
email_addr_normalized_domain(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    /* Handle NULL input */
    if (email == NULL)
        PG_RETURN_NULL();

    /* Allocate result text */
    text *result = palloc(VARHDRSZ + email->domain_len);
    SET_VARSIZE(result, VARHDRSZ + email->domain_len);

    /* Get normalized (lowercase) domain */
    unsigned char *normalized = VARDATA(result);
    const char *domain = get_domain(email);

    for (int i = 0; i < email->domain_len; i++)
        normalized[i] = pg_tolower(domain[i]);

    PG_RETURN_TEXT_P(result);
}

/*
 * Return normalized form of an email address:
 * - domain part is converted to lowercase
 * - quotes are removed if possible
 * - local part case is preserved for quoted forms that must remain quoted
 * - local part is converted to lowercase for unquoted forms
 */
PG_FUNCTION_INFO_V1(email_addr_normalize);

Datum
email_addr_normalize(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    /* Handle NULL input */
    if (email == NULL)
        PG_RETURN_NULL();

    /* Get normalized parts using existing functions */
    text *norm_local = DatumGetTextPP(DirectFunctionCall1(email_addr_normalized_local_part,
        PointerGetDatum(email)));
    text *norm_domain = DatumGetTextPP(DirectFunctionCall1(email_addr_normalized_domain,
        PointerGetDatum(email)));

    /* Calculate total size needed */
    const size_t local_len = VARSIZE_ANY_EXHDR(norm_local);
    const size_t domain_len = VARSIZE_ANY_EXHDR(norm_domain);
    const size_t total_size = VARHDRSZ +
                              offsetof(EMAIL_ADDR, data) +
                              local_len + 1 +
                              domain_len + 1;

    /* Allocate result */
    EMAIL_ADDR *result = palloc0(total_size);
    SET_VARSIZE(result, total_size);

    /* Set lengths and offset */
    result->local_len = local_len;
    result->domain_len = domain_len;
    result->domain_offset = local_len + 1;

    /* Copy data and add null terminators */
    memcpy(result->data, VARDATA_ANY(norm_local), local_len);
    result->data[local_len] = '\0';
    memcpy(result->data + result->domain_offset, VARDATA_ANY(norm_domain), domain_len);
    result->data[result->domain_offset + domain_len] = '\0';

    /* Free intermediate results */
    pfree(norm_local);
    pfree(norm_domain);

    PG_RETURN_POINTER(result);
}

/*
 * Get normalized email as text
 */
PG_FUNCTION_INFO_V1(email_addr_normalize_text);

Datum
email_addr_normalize_text(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    /* Handle NULL input */
    if (email == NULL)
        PG_RETURN_NULL();

    /* Get normalized parts */
    text *norm_local = DatumGetTextPP(DirectFunctionCall1(email_addr_normalized_local_part,
        PointerGetDatum(email)));
    text *norm_domain = DatumGetTextPP(DirectFunctionCall1(email_addr_normalized_domain,
        PointerGetDatum(email)));

    /* Calculate total length including @ */
    const int total_len = VARSIZE_ANY_EXHDR(norm_local) + 1 + VARSIZE_ANY_EXHDR(norm_domain);

    /* Allocate result text */
    text *result = palloc(VARHDRSZ + total_len);
    SET_VARSIZE(result, VARHDRSZ + total_len);

    /* Build result string */
    char *dest = VARDATA(result);
    memcpy(dest,
           VARDATA_ANY(norm_local),
           VARSIZE_ANY_EXHDR(norm_local));
    dest += VARSIZE_ANY_EXHDR(norm_local);
    *dest++ = '@';
    memcpy(dest,
           VARDATA_ANY(norm_domain),
           VARSIZE_ANY_EXHDR(norm_domain));

    /* Free intermediate results */
    pfree(norm_local);
    pfree(norm_domain);

    PG_RETURN_TEXT_P(result);
}

/*
 * Check if two email addresses are equal after normalization
 */
PG_FUNCTION_INFO_V1(email_addr_normalize_eq);

Datum
email_addr_normalize_eq(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *addr1 = PG_GETARG_EMAIL_ADDR_PP(0);
    const EMAIL_ADDR *addr2 = PG_GETARG_EMAIL_ADDR_PP(1);
    /* Handle NULLs */
    if (addr1 == NULL || addr2 == NULL) {
        PG_RETURN_NULL();
    }
    /* Normalize both addresses */
    EMAIL_ADDR *norm1 = (EMAIL_ADDR *) DatumGetPointer(DirectFunctionCall1(email_addr_normalize,
                                                                           PointerGetDatum(addr1)));
    EMAIL_ADDR *norm2 = (EMAIL_ADDR *) DatumGetPointer(DirectFunctionCall1(email_addr_normalize,
                                                                           PointerGetDatum(addr2)));
    /* Compare normalized forms */
    const bool result = email_addr_equals(norm1, norm2);
    /* Free normalized addresses */
    pfree(norm1);
    pfree(norm2);
    PG_RETURN_BOOL(result);
}

/*
 * Cast email_addr to text
 */
PG_FUNCTION_INFO_V1(email_addr_cast_to_text);

Datum
email_addr_cast_to_text(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    if (email == NULL)
        PG_RETURN_NULL();

    /* Calculate total length: local_part + @ + domain */
    const int total_len = email->local_len + 1 + email->domain_len;

    /* Allocate and initialize result */
    text *result = palloc(VARHDRSZ + total_len);
    SET_VARSIZE(result, VARHDRSZ + total_len);

    /* Build the string */
    char *dest = VARDATA(result);
    memcpy(dest, get_local_part(email), email->local_len);
    dest[email->local_len] = '@';
    memcpy(dest + email->local_len + 1, get_domain(email), email->domain_len);

    PG_RETURN_TEXT_P(result);
}

/*
 * Cast text to email_addr
 */
PG_FUNCTION_INFO_V1(text_cast_to_email_addr);

Datum
text_cast_to_email_addr(PG_FUNCTION_ARGS) {
    const text *txt = PG_GETARG_TEXT_PP(0);

    if (txt == NULL)
        PG_RETURN_NULL();

    /* Convert text to cstring and call the input function */
    char *str = text_to_cstring(txt);
    const Datum result = DirectFunctionCall1(email_addr_in, CStringGetDatum(str));

    pfree(str);

    PG_RETURN_DATUM(result);
}

/*
 * Cast email_addr to varchar
 */
PG_FUNCTION_INFO_V1(email_addr_cast_to_varchar);

Datum
email_addr_cast_to_varchar(PG_FUNCTION_ARGS) {
    /* Just use the text cast - varchar is identical internally */
    return email_addr_cast_to_text(fcinfo);
}

/*
 * Cast varchar to email_addr
 */
PG_FUNCTION_INFO_V1(varchar_cast_to_email_addr);

Datum
varchar_cast_to_email_addr(PG_FUNCTION_ARGS) {
    /* Just use the text cast - varchar is identical internally */
    return text_cast_to_email_addr(fcinfo);
}

/*
 * Cast email_addr to name
 */
PG_FUNCTION_INFO_V1(email_addr_cast_to_name);

Datum
email_addr_cast_to_name(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *email = PG_GETARG_EMAIL_ADDR_PP(0);

    if (email == NULL)
        PG_RETURN_NULL();

    /* Get as text first */
    text *txt = DatumGetTextPP(DirectFunctionCall1(email_addr_cast_to_text,
        PG_GETARG_DATUM(0)));

    /* Check length */
    int total_len = VARSIZE_ANY_EXHDR(txt);
    if (total_len >= NAMEDATALEN)
        ereport(ERROR,
            (errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
                errmsg("email address too long for type name"),
                errdetail("Email addresses cast to name type must be less than %d bytes.",
                    NAMEDATALEN)));

    /* Convert to name */
    Name result = palloc(NAMEDATALEN);
    memcpy(NameStr(*result), VARDATA_ANY(txt), total_len);
    NameStr(*result)[total_len] = '\0';

    pfree(txt);

    PG_RETURN_NAME(result);
}

/*
 * Cast name to email_addr
 */
PG_FUNCTION_INFO_V1(name_cast_to_email_addr);

Datum
name_cast_to_email_addr(PG_FUNCTION_ARGS) {
    const Name name = PG_GETARG_NAME(0);

    if (name == NULL)
        PG_RETURN_NULL();

    /* Convert name to text */
    text *txt = cstring_to_text(NameStr(*name));

    /* Use text cast */
    const Datum result = DirectFunctionCall1(text_cast_to_email_addr,
                                             PointerGetDatum(txt));

    pfree(txt);

    PG_RETURN_DATUM(result);
}

/*
 * Compare email address by domain
 */
PG_FUNCTION_INFO_V1(email_addr_domain_cmp);

Datum
email_addr_domain_cmp(PG_FUNCTION_ARGS) {
    const EMAIL_ADDR *addr1 = PG_GETARG_EMAIL_ADDR_PP(0);
    const EMAIL_ADDR *addr2 = PG_GETARG_EMAIL_ADDR_PP(1);
    int cmp;

    /* Handle NULLs */
    if (addr1 == NULL) {
        if (addr2 == NULL)
            PG_RETURN_INT32(0); /* NULL == NULL */
        PG_RETURN_INT32(-1); /* NULL < non-NULL */
    }
    if (addr2 == NULL)
        PG_RETURN_INT32(1); /* non-NULL > NULL */

    /* Get normalized (lowercase) domains */
    text *domain1 = DatumGetTextPP(DirectFunctionCall1(email_addr_normalized_domain,
        PG_GETARG_DATUM(0)));
    text *domain2 = DatumGetTextPP(DirectFunctionCall1(email_addr_normalized_domain,
        PG_GETARG_DATUM(1)));

    /* Compare the domains */
    if (VARSIZE_ANY_EXHDR(domain1) < VARSIZE_ANY_EXHDR(domain2))
        cmp = -1;
    else if (VARSIZE_ANY_EXHDR(domain1) > VARSIZE_ANY_EXHDR(domain2))
        cmp = 1;
    else {
        cmp = memcmp(VARDATA_ANY(domain1), VARDATA_ANY(domain2),
                     VARSIZE_ANY_EXHDR(domain1));
    }

    pfree(domain1);
    pfree(domain2);

    PG_RETURN_INT32(cmp);
}

/*
 * Domain equality operator
 */
PG_FUNCTION_INFO_V1(email_addr_domain_eq);

Datum
email_addr_domain_eq(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_domain_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp == 0);
}

/*
 * Domain not equal operator
 */
PG_FUNCTION_INFO_V1(email_addr_domain_ne);

Datum
email_addr_domain_ne(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_domain_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp != 0);
}

/*
 * Domain less than operator
 */
PG_FUNCTION_INFO_V1(email_addr_domain_lt);

Datum
email_addr_domain_lt(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_domain_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp < 0);
}

/*
 * Domain less than or equal operator
 */
PG_FUNCTION_INFO_V1(email_addr_domain_le);

Datum
email_addr_domain_le(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_domain_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp <= 0);
}

/*
 * Domain greater than operator
 */
PG_FUNCTION_INFO_V1(email_addr_domain_gt);

Datum
email_addr_domain_gt(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_domain_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp > 0);
}

/*
 * Domain greater than or equal operator
 */
PG_FUNCTION_INFO_V1(email_addr_domain_ge);

Datum
email_addr_domain_ge(PG_FUNCTION_ARGS) {
    const int32 cmp = DatumGetInt32(DirectFunctionCall2(email_addr_domain_cmp,
                                                        PG_GETARG_DATUM(0),
                                                        PG_GETARG_DATUM(1)));
    PG_RETURN_BOOL(cmp >= 0);
}
