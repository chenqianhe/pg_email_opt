DROP EXTENSION IF EXISTS pg_email_opt CASCADE;
CREATE EXTENSION pg_email_opt;

DROP TABLE IF EXISTS email_test;
CREATE TABLE email_test
(
    id          serial PRIMARY KEY,
    email       email_addr,
    description text
);
