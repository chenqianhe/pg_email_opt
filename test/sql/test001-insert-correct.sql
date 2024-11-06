-- =====================
-- Valid Test Cases
-- =====================

-- Basic valid formats (100% valid, most commonly used patterns)
INSERT INTO email_test (email, description)
VALUES ('simple@example.com', 'Basic email format'),
       ('very.common@example.com', 'Common format with dot'),
       ('firstname.lastname@example.com', 'Standard name format'),
       ('user.name+tag@example.com', 'Plus tag format common in Gmail'),
       ('test123@example.com', 'Alphanumeric local part');

-- Special characters in local part (valid but less common)
INSERT INTO email_test (email, description)
VALUES ('first_last@example.com', 'Underscore separator'),
       ('first-last@example.com', 'Hyphen separator'),
       ('user+tag+sorting@example.com', 'Multiple plus signs'),
       ('customer/department@example.com', 'Forward slash in local part'),
       ('$A12345@example.com', 'Dollar sign in local part'),
       ('!def!xyz%abc@example.com', 'Special chars in local part');

-- Valid quoted local parts (valid but rarely used)
INSERT INTO email_test (email, description)
VALUES ('"john.doe"@example.com', 'Simple quoted name'),
       ('"john..doe"@example.org', 'Quoted double dots allowed'),
       ('" "@example.org', 'Quoted space as local part'),
       ('"very.unusual.@.unusual.com"@example.com', 'Special chars in quotes'),
       ('"foo@bar"@example.com', 'Quoted @ in local part');

-- Domain variations (all valid)
INSERT INTO email_test (email, description)
VALUES ('email@example.com', 'Common domain'),
       ('email@subdomain.example.com', 'One subdomain'),
       ('email@example-one.com', 'Hyphen in domain'),
       ('email@example.co.jp', 'Country code second-level domain'),
       ('email@[123.123.123.123]', 'IPv4 literal address');

-- Length edge cases (valid but near limits)
INSERT INTO email_test (email, description)
VALUES ('a@example.com', 'Shortest valid local part'),
       ('abcdefghijklmnopqrstuvwxyz@example.com', 'Long but valid local part'),
       ('test@a.b.c.d.example.com', 'Multiple short subdomains');

-- Case sensitivity (valid)
INSERT INTO email_test (email, description)
VALUES ('Test.Email@Example.Com', 'Mixed case everything'),
       ('test.email@example.com', 'All lowercase'),
       ('POSTMASTER@EXAMPLE.COM', 'Special postmaster address');
