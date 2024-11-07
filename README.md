# pg_email_opt

`pg_email_opt` is a PostgreSQL extension that provides an optimized data type for storing and working with email addresses. It implements RFC-compliant email address handling with efficient storage, domain-based operations, and comprehensive indexing support.

## Features

- **Optimized Storage**: Efficiently stores email addresses using a custom varlena type
- **RFC Compliance**: Follows RFC 5321/5322 rules for email address handling
- **Domain-based Operations**: Enables filtering and sorting by email domains
- **Normalization Support**: Provides functions for email address normalization
- **Comprehensive Indexing**: Supports both B-tree and hash indexes
- **Type Safety**: Ensures email address validity at input time
- **Rich Comparison Operations**: Full set of comparison operators for both full addresses and domains
- **Implicit Type Casting**: Seamless integration with text, varchar, and name types

## Usage

### Basic Usage

```sql
-- Create a table with email_addr column
CREATE TABLE users (
    id serial PRIMARY KEY,
    email email_addr NOT NULL
);

-- Insert email addresses
INSERT INTO users (email) VALUES 
    ('user@example.com'),
    ('admin@example.com'),
    ('contact@other-domain.com');

-- Query by full email
SELECT * FROM users WHERE email = 'user@example.com';

-- Query by domain using domain comparison operators
SELECT * FROM users WHERE email =#  'anything@example.com';
```

### Operators

#### Standard Comparison Operators
- `=` - Equality
- `<>` - Inequality
- `<` - Less than
- `<=` - Less than or equal
- `>` - Greater than
- `>=` - Greater than or equal

#### Domain-based Operators
- `=#` - Domain equality
- `<>#` - Domain inequality
- `<#` - Domain less than
- `<=#` - Domain less than or equal
- `>#` - Domain greater than
- `>=#` - Domain greater than or equal

#### Normalization Operator
- `==#` - Normalized equality (case-insensitive, handles quoted parts)

### Functions

- `email_addr_get_local_part(email_addr)` - Extract local part
- `email_addr_get_domain(email_addr)` - Extract domain
- `email_addr_normalize(email_addr)` - Get normalized form
- `email_addr_normalize_text(email_addr)` - Get normalized text representation
- `email_addr_normalized_local_part(email_addr)` - Get normalized local part
- `email_addr_normalized_domain(email_addr)` - Get normalized domain

### Indexing

```sql
-- B-tree index on full email address
CREATE INDEX users_email_idx ON users USING btree (email);

-- B-tree index on email domains
CREATE INDEX users_email_domain_idx ON users USING btree (email email_addr_domain_ops);

-- Hash index
CREATE INDEX users_email_hash_idx ON users USING hash (email);
```

## Implementation Details

### Storage Format

The extension uses a custom varlena type (`EMAIL_ADDR`) with the following structure:

```c
typedef struct {
    char        vl_len_[4];        /* varlena header */
    uint16      domain_offset;     /* offset to domain part */
    uint16      local_len;         /* length of local part */
    uint16      domain_len;        /* length of domain part */
    char        data[];            /* actual email data */
} EMAIL_ADDR;
```

This structure provides:
- Efficient TOAST support through varlena
- Quick access to local and domain parts
- Minimal memory overhead
- Alignment-optimized layout

### Normalization Rules

Email address normalization follows these rules:
1. Domains are converted to lowercase
2. Unquoted local parts are preserved as-is
3. Quoted local parts are unquoted if possible
4. Comments are removed

## Source

- Email Syntax: https://www.wikiwand.com/en/articles/Email_address

