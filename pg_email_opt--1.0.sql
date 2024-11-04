-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_email_opt" to load this file. \quit

-- Create the email_addr type
CREATE TYPE email_addr;

-- Create input/output functions
CREATE FUNCTION email_addr_in(cstring)
    RETURNS email_addr
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_out(email_addr)
    RETURNS cstring
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Register the type with its I/O functions
CREATE TYPE email_addr (
    INTERNALLENGTH = VARIABLE,
    INPUT = email_addr_in,
    OUTPUT = email_addr_out,
    STORAGE = EXTENDED
);

COMMENT ON TYPE email_addr IS 'Email address data type with optimized storage and domain-based operations';

-- Comparison functions
CREATE FUNCTION email_addr_lt(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_le(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_eq(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_ne(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_ge(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_gt(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Comparison operators
CREATE OPERATOR < (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_lt,
    COMMUTATOR = >,
    NEGATOR = >=,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR <= (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_le,
    COMMUTATOR = >=,
    NEGATOR = >,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR = (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_eq,
    COMMUTATOR = =,
    NEGATOR = <>,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    HASHES,
    MERGES
);

CREATE OPERATOR <> (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_ne,
    COMMUTATOR = <>,
    NEGATOR = =,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);

CREATE OPERATOR >= (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_ge,
    COMMUTATOR = <=,
    NEGATOR = <,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_gt,
    COMMUTATOR = <,
    NEGATOR = <=,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

-- Hash function for hash indexes
CREATE FUNCTION email_hash(email_addr)
    RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- B-tree operator class for email_addr
CREATE OPERATOR CLASS email_addr_ops
DEFAULT FOR TYPE email_addr USING btree AS
    OPERATOR    1   <,
    OPERATOR    2   <=,
    OPERATOR    3   =,
    OPERATOR    4   >=,
    OPERATOR    5   >,
    FUNCTION    1   email_addr_cmp(email_addr, email_addr);

-- Hash operator class for email_addr
CREATE OPERATOR CLASS email_addr_hash_ops
DEFAULT FOR TYPE email_addr USING hash AS
    OPERATOR    1   =,
    FUNCTION    1   email_hash(email_addr);

-- Domain-based comparison functions
CREATE FUNCTION email_addr_domain_lt(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_domain_le(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_domain_eq(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_domain_ne(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_domain_ge(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_domain_gt(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Domain-based comparison operators
CREATE OPERATOR <# (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_domain_lt,
    COMMUTATOR = >#,
    NEGATOR = >=#,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR <=# (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_domain_le,
    COMMUTATOR = >=#,
    NEGATOR = >#,
    RESTRICT = scalarltsel,
    JOIN = scalarltjoinsel
);

CREATE OPERATOR =# (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_domain_eq,
    COMMUTATOR = =#,
    NEGATOR = <>#,
    RESTRICT = eqsel,
    JOIN = eqjoinsel,
    MERGES
);

CREATE OPERATOR <>## (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_domain_ne,
    COMMUTATOR = <>#,
    NEGATOR = =#,
    RESTRICT = neqsel,
    JOIN = neqjoinsel
);

CREATE OPERATOR >=# (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_domain_ge,
    COMMUTATOR = <=#,
    NEGATOR = <#,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

CREATE OPERATOR ># (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_domain_gt,
    COMMUTATOR = <#,
    NEGATOR = <=#,
    RESTRICT = scalargtsel,
    JOIN = scalargtjoinsel
);

-- Domain-based B-tree operator class
CREATE OPERATOR CLASS email_addr_domain_ops
FOR TYPE email_addr USING btree AS
    OPERATOR    1   <#,
    OPERATOR    2   <=#,
    OPERATOR    3   =#,
    OPERATOR    4   >=#,
    OPERATOR    5   >#,
    FUNCTION    1   email_addr_domain_cmp(email_addr, email_addr);

-- Accessor functions
CREATE FUNCTION email_addr_get_local_part(email_addr)
    RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_get_domain(email_addr)
    RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Normalization functions
CREATE FUNCTION email_addr_normalized_local_part(email_addr)
    RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_normalized_domain(email_addr)
    RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_normalize(email_addr)
    RETURNS email_addr
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_addr_normalize_text(email_addr)
    RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Normalization comparison
CREATE FUNCTION email_addr_normalize_eq(email_addr, email_addr)
    RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Normalization equality operator
CREATE OPERATOR ==# (
    LEFTARG = email_addr,
    RIGHTARG = email_addr,
    PROCEDURE = email_addr_normalize_eq,
    COMMUTATOR = ==#,
    NEGATOR = <>##,
    RESTRICT = eqsel,
    JOIN = eqjoinsel
);

-- Type casts
CREATE FUNCTION email_addr_cast_to_text(email_addr)
    RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION text_cast_to_email_addr(text)
    RETURNS email_addr
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE CAST (email_addr AS text)
    WITH FUNCTION email_addr_cast_to_text(email_addr)
AS IMPLICIT;

CREATE CAST (text AS email_addr)
    WITH FUNCTION text_cast_to_email_addr(text)
AS IMPLICIT;

-- VARCHAR casts
CREATE FUNCTION email_addr_cast_to_varchar(email_addr)
    RETURNS varchar
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION varchar_cast_to_email_addr(varchar)
    RETURNS email_addr
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE CAST (email_addr AS varchar)
    WITH FUNCTION email_addr_cast_to_varchar(email_addr)
AS IMPLICIT;

CREATE CAST (varchar AS email_addr)
    WITH FUNCTION varchar_cast_to_email_addr(varchar)
AS IMPLICIT;

-- NAME type casts
CREATE FUNCTION email_addr_cast_to_name(email_addr)
    RETURNS name
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION name_cast_to_email_addr(name)
    RETURNS email_addr
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE CAST (email_addr AS name)
    WITH FUNCTION email_addr_cast_to_name(email_addr)
AS ASSIGNMENT;

CREATE CAST (name AS email_addr)
    WITH FUNCTION name_cast_to_email_addr(name)
AS ASSIGNMENT;

-- Add helpful comments
COMMENT ON TYPE email_addr IS 'Email address data type with optimized storage and domain-based operations';
COMMENT ON FUNCTION email_addr_in(cstring) IS 'Convert string to email_addr';
COMMENT ON FUNCTION email_addr_out(email_addr) IS 'Convert email_addr to string';
COMMENT ON FUNCTION email_addr_get_local_part(email_addr) IS 'Extract local part from email address';
COMMENT ON FUNCTION email_addr_get_domain(email_addr) IS 'Extract domain part from email address';
COMMENT ON FUNCTION email_addr_normalize(email_addr) IS 'Normalize email address according to RFC rules';
COMMENT ON FUNCTION email_addr_normalize_text(email_addr) IS 'Convert email address to normalized text form';
COMMENT ON OPERATOR <# (email_addr, email_addr) IS 'Domain-based less than comparison';
COMMENT ON OPERATOR <=# (email_addr, email_addr) IS 'Domain-based less than or equal comparison';
COMMENT ON OPERATOR =# (email_addr, email_addr) IS 'Domain-based equality comparison';
COMMENT ON OPERATOR >=# (email_addr, email_addr) IS 'Domain-based greater than or equal comparison';
COMMENT ON OPERATOR ># (email_addr, email_addr) IS 'Domain-based greater than comparison';
COMMENT ON OPERATOR ==# (email_addr, email_addr) IS 'Normalized email address equality comparison';
