-- Test Case Group 1: Local part format violations
-- 1.1: Missing @ character
INSERT INTO email_test (email, description)
VALUES ('abc.example.com', 'Missing @ character');

-- 1.2: Multiple @ characters
INSERT INTO email_test (email, description)
VALUES ('a@b@c@example.com', 'Multiple @ characters');

-- 1.3: Leading dot in local part
INSERT INTO email_test (email, description)
VALUES ('.test@example.com', 'Leading dot in local part');

-- 1.4: Trailing dot in local part
INSERT INTO email_test (email, description)
VALUES ('test.@example.com', 'Trailing dot in local part');

-- 1.5: Consecutive dots in local part
INSERT INTO email_test (email, description)
VALUES ('te..st@example.com', 'Consecutive dots in local part');

-- 1.6: Unquoted space in local part
INSERT INTO email_test (email, description)
VALUES ('test space@example.com', 'Unquoted space in local part');

-- Test Case Group 2: Invalid quoting
-- 2.1: Unescaped quotes
INSERT INTO email_test (email, description)
VALUES ('just"not"right@example.com', 'Unescaped quotes');

-- 2.2: Space and quote issues
INSERT INTO email_test (email, description)
VALUES ('this is"not\allowed@example.com', 'Space and quote issues');

-- 2.3: Invalid escaping
INSERT INTO email_test (email, description)
VALUES ('this\ still\"not\\allowed@example.com', 'Invalid escaping');

-- Test Case Group 3: Length violations
-- 3.1: Local part > 64 chars
INSERT INTO email_test (email, description)
VALUES ('1234567890123456789012345678901234567890123456789012345678901234+x@example.com',
        'Local part > 64 chars');

-- 3.2: Domain part too long
INSERT INTO email_test (email, description)
VALUES ('test@' || repeat('a', 255) || '.com', 'Domain part too long');

-- Test Case Group 4: Domain format violations
-- 4.1: Consecutive dots in domain
INSERT INTO email_test (email, description)
VALUES ('test@example..com', 'Consecutive dots in domain');

-- 4.2: Leading hyphen in domain label
INSERT INTO email_test (email, description)
VALUES ('test@-example.com', 'Leading hyphen in domain label');

-- 4.3: Trailing hyphen in domain label
INSERT INTO email_test (email, description)
VALUES ('test@example-.com', 'Trailing hyphen in domain label');

-- 4.4: Leading dot in domain
INSERT INTO email_test (email, description)
VALUES ('test@.example.com', 'Leading dot in domain');

-- 4.5: Trailing dot in domain
INSERT INTO email_test (email, description)
VALUES ('test@example.com.', 'Trailing dot in domain');

-- 4.6: Space in domain
INSERT INTO email_test (email, description)
VALUES ('test@exam ple.com', 'Space in domain');

-- 4.7: Underscore in domain
INSERT INTO email_test (email, description)
VALUES ('test@exa_mple.com', 'Underscore in domain');

-- Test Case Group 5: Character set violations
-- 5.1: Non-ASCII in local part without SMTPUTF8
INSERT INTO email_test (email, description)
VALUES ('тест@example.com', 'Non-ASCII in local part without SMTPUTF8');

-- 5.2: Non-ASCII in domain without IDNA
INSERT INTO email_test (email, description)
VALUES ('test@测试.com', 'Non-ASCII in domain without IDNA');

-- 5.3: Invalid IP format
INSERT INTO email_test (email, description)
VALUES ('test@[333.333.333.333]', 'Invalid IP format');

-- 5.4: Invalid IPv6 format
INSERT INTO email_test (email, description)
VALUES ('test@[IPv6:2001:db8:85a3::8a2e:37023:7334]', 'Invalid IPv6 format');

-- Test Case Group 6: Syntax combination errors
-- 6.1: Invalid quote combination
INSERT INTO email_test (email, description)
VALUES ('"test"test"@example.com', 'Invalid quote combination');

-- 6.2: Unmatched quote
INSERT INTO email_test (email, description)
VALUES ('test"@example.com', 'Unmatched quote');

-- 6.3: Missing domain part
INSERT INTO email_test (email, description)
VALUES ('test@', 'Missing domain part');

-- 6.4: Missing local part
INSERT INTO email_test (email, description)
VALUES ('@example.com', 'Missing local part');

-- Test Case Group 7: Comment-related errors
-- 7.1: Leading comment
INSERT INTO email_test (email, description)
VALUES ('(comment)test@example.com', 'Leading comment not supported');

-- 7.2: Embedded comment
INSERT INTO email_test (email, description)
VALUES ('test(comment)@example.com', 'Embedded comment not supported');

-- 7.3: Trailing comment
INSERT INTO email_test (email, description)
VALUES ('test@example.com(comment)', 'Trailing comment not supported');
