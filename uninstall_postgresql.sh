#!/bin/bash

# Must run as root
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

echo "Starting PostgreSQL complete uninstallation..."

# 1. Stop all PostgreSQL services
echo "Stopping PostgreSQL services..."
if command -v systemctl >/dev/null 2>&1; then
    systemctl stop postgresql*
    systemctl disable postgresql*
elif command -v service >/dev/null 2>&1; then
    service postgresql stop
fi

# 2. Remove PostgreSQL packages
echo "Removing PostgreSQL packages..."
if command -v apt-get >/dev/null 2>&1; then
    # Debian/Ubuntu
    apt-get remove --purge -y postgresql*
    apt-get remove --purge -y pgadmin*
    apt-get autoremove -y
elif command -v yum >/dev/null 2>&1; then
    # RHEL/CentOS
    yum remove -y postgresql*
    yum remove -y pgadmin*
elif command -v dnf >/dev/null 2>&1; then
    # Fedora
    dnf remove -y postgresql*
    dnf remove -y pgadmin*
elif command -v zypper >/dev/null 2>&1; then
    # openSUSE
    zypper remove -y postgresql*
    zypper remove -y pgadmin*
fi

# 3. Remove PostgreSQL directories
echo "Removing PostgreSQL directories..."
rm -rf /var/lib/postgresql/
rm -rf /var/log/postgresql/
rm -rf /etc/postgresql/
rm -rf /usr/lib/postgresql/
rm -rf /usr/share/postgresql/
rm -rf /usr/share/doc/postgresql/

# 4. Remove PostgreSQL user and group
echo "Removing PostgreSQL user and group..."
userdel -r postgres >/dev/null 2>&1
groupdel postgres >/dev/null 2>&1

# 5. Remove PostgreSQL configuration files
echo "Removing configuration files..."
rm -rf ~/.psql_history
rm -rf ~/.psqlrc
rm -f /root/.psql_history
rm -f /root/.psqlrc

# 6. Clean package manager cache
echo "Cleaning package manager cache..."
if command -v apt-get >/dev/null 2>&1; then
    apt-get clean
elif command -v yum >/dev/null 2>&1; then
    yum clean all
elif command -v dnf >/dev/null 2>&1; then
    dnf clean all
elif command -v zypper >/dev/null 2>&1; then
    zypper clean
fi

# 7. Remove any remaining config files
find /etc -name "*postgres*" -delete
find /etc -name "*pgadmin*" -delete

# 8. Remove data directory if exists in custom location
if [ -d "/usr/local/pgsql/data" ]; then
    rm -rf /usr/local/pgsql/data
fi

# 9. Remove any remaining compiled versions
rm -rf /usr/local/pgsql

# 10. Clean environment variables
if [ -f /etc/profile.d/postgres.sh ]; then
    rm -f /etc/profile.d/postgres.sh
fi

echo "PostgreSQL uninstallation completed."
echo "Note: System reboot recommended to ensure all components are fully removed."