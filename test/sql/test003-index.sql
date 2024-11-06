-- ================================================
-- Comprehensive Index Testing for email_addr Type
-- ================================================

-- Clean up any existing indexes
DROP INDEX IF EXISTS idx_email_btree;
DROP INDEX IF EXISTS idx_email_hash;
DROP INDEX IF EXISTS idx_email_domain_btree;
DROP INDEX IF EXISTS idx_email_local_btree;

-- ------------------------------------------------
-- Create different types of indexes
-- ------------------------------------------------

-- Standard B-tree index for full email comparison
CREATE INDEX idx_email_btree ON email_test USING btree(email);

-- Hash index for equality comparisons
CREATE INDEX idx_email_hash ON email_test USING hash(email);

-- B-tree index using domain-based operator class
CREATE INDEX idx_email_domain_btree ON email_test USING btree(email email_addr_domain_ops);

-- B-tree index on extracted domain for domain-specific queries
CREATE INDEX idx_email_domain_extract ON email_test USING btree((email_addr_get_domain(email)));

-- Analyze the table to update statistics
ANALYZE email_test;

-- ------------------------------------------------
-- Test 1: Basic Index Usage for Equality
-- ------------------------------------------------

-- Should use hash index
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email = 'simple@example.com';

-- Should use btree index
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email = 'simple@example.com'
ORDER BY email;

-- ------------------------------------------------
-- Test 2: Range Queries
-- ------------------------------------------------

-- Should use btree index for range scan
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email > 'n@example.com'
ORDER BY email;

-- Should use btree index for BETWEEN
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email BETWEEN 'a@example.com' AND 'n@example.com'
ORDER BY email;

-- ------------------------------------------------
-- Test 3: Domain-Based Operations
-- ------------------------------------------------

-- Should use domain-specific index
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email ># 'test@example.com'
ORDER BY email;

-- Domain extraction index usage
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email_addr_get_domain(email) = 'example.com'
ORDER BY email;

-- ------------------------------------------------
-- Test 4: Sorting Performance
-- ------------------------------------------------

-- Regular email sorting
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
ORDER BY email;

-- Domain-then-local sorting
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
ORDER BY email_addr_get_domain(email), email_addr_get_local_part(email);

-- ------------------------------------------------
-- Test 5: Index Only Scans
-- ------------------------------------------------

-- Test if index-only scan is possible
EXPLAIN (ANALYZE, BUFFERS)
SELECT email
FROM email_test
WHERE email > 'n@example.com'
ORDER BY email;

-- ------------------------------------------------
-- Test 6: Composite Key Usage
-- ------------------------------------------------

-- Create composite index with id
CREATE INDEX idx_email_composite ON email_test(email, id);

-- Test composite index usage
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, id, description
FROM email_test
WHERE email = 'simple@example.com'
  AND id < 1000
ORDER BY email, id;

-- ------------------------------------------------
-- Test 7: Pattern Matching with Indexes
-- ------------------------------------------------

-- Test like patterns (might not use index effectively)
EXPLAIN (ANALYZE, BUFFERS)
SELECT email, description
FROM email_test
WHERE email::text LIKE '%@example.com'
ORDER BY email;

-- ------------------------------------------------
-- Test 8: Index Size and Statistics
-- ------------------------------------------------

-- Show index sizes and usage statistics
SELECT
    schemaname,
    tablename,
    indexname,
    indexdef,
    pg_size_pretty(pg_relation_size(quote_ident(schemaname) || '.' || quote_ident(indexname)::regclass)) as index_size
FROM pg_indexes
WHERE tablename = 'email_test'
ORDER BY indexname;

-- Show index usage statistics (if pg_stat_user_indexes is available)
SELECT
    schemaname,
    relname,
    indexrelname,
    idx_scan,
    idx_tup_read,
    idx_tup_fetch
FROM pg_stat_user_indexes
WHERE relname = 'email_test'
ORDER BY indexrelname;

-- ------------------------------------------------
-- Test 9: Cleanup (Optional)
-- ------------------------------------------------

DROP INDEX IF EXISTS idx_email_btree;
DROP INDEX IF EXISTS idx_email_hash;
DROP INDEX IF EXISTS idx_email_domain_btree;
DROP INDEX IF EXISTS idx_email_domain_extract;
DROP INDEX IF EXISTS idx_email_composite;
