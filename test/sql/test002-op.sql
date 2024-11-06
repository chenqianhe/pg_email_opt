-- ========= Test Group 1: Basic Tests and Sample Data =========

-- Count total records and show distribution
SELECT count(*) as total_records FROM email_test;

-- Show sample of test data
SELECT email, description
FROM email_test
ORDER BY email
LIMIT 5;

-- ========= Test Group 2: Standard Comparison Operators =========
-- Test equality operator
SELECT email, description
FROM email_test
WHERE email = 'user@example.com';

-- Test inequality operator
SELECT email, description
FROM email_test
WHERE email <> 'user@example.com'
LIMIT 5;

-- Test less than operator
SELECT email, description
FROM email_test
WHERE email < 'n@example.com'
ORDER BY email
LIMIT 5;

-- Test greater than operator
SELECT email, description
FROM email_test
WHERE email > 'n@example.com'
ORDER BY email
LIMIT 5;

-- Test range queries
SELECT email, description
FROM email_test
WHERE email BETWEEN 'a@example.com' AND 'c@example.com'
ORDER BY email;

-- ========= Test Group 3: Domain-Based Comparison Operators =========
-- Test domain equality (=# operator)
SELECT email, description
FROM email_test
WHERE email =# 'anyuser@EXAMPLE.COM'  -- Domain comparison is case-insensitive
ORDER BY email;

-- Test domain inequality (<># operator)
SELECT email, description
FROM email_test
WHERE email <>## 'anyuser@example.com'
ORDER BY email
LIMIT 5;

-- Test domain ordering (<# operator)
SELECT email, description
FROM email_test
WHERE email <# 'user@gmail.com'  -- Emails with domains alphabetically before gmail.com
ORDER BY email
LIMIT 5;

-- Test domain ordering (># operator)
SELECT email, description
FROM email_test
WHERE email ># 'user@gmail.com'  -- Emails with domains alphabetically after gmail.com
ORDER BY email
LIMIT 5;

-- Show domain-based sorting
SELECT email, email_addr_get_domain(email) as domain
FROM email_test
ORDER BY email_addr_get_domain(email), email
LIMIT 10;

-- ========= Test Group 4: Advanced Domain Operations =========
-- Count emails per domain (case-insensitive)
SELECT
    email_addr_normalized_domain(email) as normalized_domain,
    count(*) as email_count
FROM email_test
GROUP BY normalized_domain
ORDER BY email_count DESC;

-- Find domains with mixed case variations
SELECT DISTINCT
    email_addr_get_domain(email) as original_domain,
    email_addr_normalized_domain(email) as normalized_domain
FROM email_test
WHERE email_addr_get_domain(email) <> email_addr_normalized_domain(email)::text
ORDER BY normalized_domain;

-- Group emails by top-level domain
SELECT
    substring(email_addr_normalized_domain(email) from '[^.]+$') as tld,
    count(*) as domain_count
FROM email_test
GROUP BY tld
ORDER BY domain_count DESC;

-- ========= Test Group 5: Normalization and Comparison Edge Cases =========
-- Test normalized equality operator (==# operator)
SELECT
    a.email as email1,
    b.email as email2,
    a.email ==# b.email as normalized_equal
FROM email_test a
         CROSS JOIN email_test b
WHERE a.email <> b.email
  AND a.email ==# b.email
LIMIT 5;

-- Show emails that differ only in case
SELECT
    a.email as email1,
    b.email as email2
FROM email_test a
         JOIN email_test b ON lower(a.email::text) = lower(b.email::text)
WHERE a.email <> b.email
LIMIT 5;

-- Test domain-based index usage
EXPLAIN ANALYZE
SELECT email, description
FROM email_test
WHERE email ># 'user@example.com'
ORDER BY email;

-- ========= Test Group 6: Special Cases and Edge Conditions =========
-- Test quoted local parts with domain comparison
SELECT email, description
FROM email_test
WHERE email::text LIKE '"%@%'
  AND email =# 'user@example.com'
ORDER BY email;

-- Test maximum length domains
SELECT email, length(email_addr_get_domain(email)) as domain_length
FROM email_test
ORDER BY domain_length DESC
LIMIT 5;

-- Test maximum length local parts
SELECT email, length(email_addr_get_local_part(email)) as local_part_length
FROM email_test
ORDER BY local_part_length DESC
LIMIT 5;

-- ========= Test Group 7: Index Usage Tests =========
-- Test btree index usage for email ordering
EXPLAIN ANALYZE
SELECT email
FROM email_test
ORDER BY email;

-- Test hash index usage for equality
EXPLAIN ANALYZE
SELECT email
FROM email_test
WHERE email = 'user@example.com';

-- Test domain-based btree index usage
EXPLAIN ANALYZE
SELECT email
FROM email_test
WHERE email ># 'user@example.com'
ORDER BY email;

-- ========= Test Group 8: Performance Comparison =========
-- Compare performance: standard vs normalized comparison
EXPLAIN ANALYZE
SELECT count(*)
FROM email_test a
         JOIN email_test b ON a.email = b.email;

EXPLAIN ANALYZE
SELECT count(*)
FROM email_test a
         JOIN email_test b ON a.email ==# b.email;

-- Compare performance: standard vs domain-based comparison
EXPLAIN ANALYZE
SELECT count(*)
FROM email_test
WHERE email > 'user@example.com';

EXPLAIN ANALYZE
SELECT count(*)
FROM email_test
WHERE email ># 'user@example.com';
