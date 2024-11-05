#!/bin/bash

# Must run as root
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

echo "Starting PostgreSQL 16 installation..."

# 1. Add PostgreSQL official repository
if command -v apt-get >/dev/null 2>&1; then
    # Debian/Ubuntu
    echo "Setting up PostgreSQL repository for Debian/Ubuntu..."
    sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
    wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add -
    apt-get update

    # Install PostgreSQL 16 specifically
    echo "Installing PostgreSQL 16..."
    apt-get install -y postgresql-16 postgresql-contrib-16 postgresql-client-16

    # Set PostgreSQL 16 as default and prevent 17 from being installed
    echo "Setting PostgreSQL 16 as default..."
    cat > /etc/apt/preferences.d/postgresql << EOF
Package: postgresql*
Pin: version 16*
Pin-Priority: 1000

Package: postgresql*
Pin: version 17*
Pin-Priority: -1
EOF

elif command -v yum >/dev/null 2>&1; then
    # RHEL/CentOS
    echo "Setting up PostgreSQL repository for RHEL/CentOS..."
    yum install -y https://download.postgresql.org/pub/repos/yum/reporpms/EL-$(rpm -E %{rhel})-x86_64/pgdg-redhat-repo-latest.noarch.rpm

    # Disable PostgreSQL module if it exists
    dnf -qy module disable postgresql

    # Install PostgreSQL 16
    echo "Installing PostgreSQL 16..."
    yum install -y postgresql16-server postgresql16-contrib postgresql16

    # Initialize database
    /usr/pgsql-16/bin/postgresql-16-setup initdb

    # Start and enable service
    systemctl enable postgresql-16
    systemctl start postgresql-16

    # Create yum repository preference file
    cat > /etc/yum.repos.d/postgresql-preference.repo << EOF
[pgdg16]
name=PostgreSQL 16
baseurl=https://download.postgresql.org/pub/repos/yum/16/redhat/rhel-\$releasever-\$basearch
enabled=1
gpgcheck=1
gpgkey=https://download.postgresql.org/pub/repos/yum/RPM-GPG-KEY-PGDG
priority=1

[pgdg17]
name=PostgreSQL 17
enabled=0
EOF

fi

# 2. Configure PostgreSQL
echo "Configuring PostgreSQL..."
PG_CONF="/etc/postgresql/16/main/postgresql.conf"
PG_HBA="/etc/postgresql/16/main/pg_hba.conf"

if [ -f "$PG_CONF" ]; then
    # Adjust PostgreSQL configuration
    sed -i "s/#listen_addresses = 'localhost'/listen_addresses = '*'/" "$PG_CONF"

    # Restart PostgreSQL
    if command -v systemctl >/dev/null 2>&1; then
        systemctl restart postgresql-16
    else
        service postgresql restart
    fi
fi

# 3. Set up environment variables
echo "Setting up environment variables..."
cat > /etc/profile.d/postgresql16.sh << EOF
export PGDATA=/var/lib/postgresql/16/main
export PATH=\$PATH:/usr/lib/postgresql/16/bin
EOF

# Source the new environment variables
source /etc/profile.d/postgresql16.sh

echo "PostgreSQL 16 installation completed."
echo "You can now connect to PostgreSQL using: sudo -u postgres psql"

# Print installation verification info
echo -e "\nVerifying installation:"
if command -v psql >/dev/null 2>&1; then
    psql --version
fi

if command -v systemctl >/dev/null 2>&1; then
    systemctl status postgresql-16 | grep "Active:"
fi
