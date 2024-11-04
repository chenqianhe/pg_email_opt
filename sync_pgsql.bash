cp cmake-build-release/lib/pg_email_opt.so $(pg_config --pkglibdir)/
cp pg_email_opt.control $(pg_config --sharedir)/extension/
cp pg_email_opt--1.0.sql $(pg_config --sharedir)/extension/

chmod 755 $(pg_config --pkglibdir)/pg_email_opt.so
chmod 644 $(pg_config --sharedir)/extension/pg_email_opt.control
chmod 644 $(pg_config --sharedir)/extension/pg_email_opt--1.0.sql
