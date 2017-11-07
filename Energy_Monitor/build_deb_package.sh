#!/bin/bash
SOFTWARE_VERSION="1.0.11"
##################################################
## Script for building the Energy Monitor Debian package
##################################################
DEB_PACKAGE_DIR=Debian_package/energy-monitor_1.0
# Copy the binary into the correct location for building the package
cp dist/Debug/GNU-Linux/Energy_Monitor $DEB_PACKAGE_DIR/usr/bin/
# Set the appropriate ownership and permissions
chown -R root:root $DEB_PACKAGE_DIR
# Set the appropriate ownership and permissions
chmod -R 0755 $DEB_PACKAGE_DIR
# Create the MD5 sums file for all files that are not in the DEBIAN dir
find $DEB_PACKAGE_DIR -type f ! -regex '.*.hg.*' ! -regex '.*?debian-binary.*' ! -regex '.*?DEBIAN.*' -printf '%p ' | xargs md5sum > $DEB_PACKAGE_DIR/DEBIAN/md5sums
# Build the package
dpkg-deb --build Debian_package/energy-monitor_1.0

########################
## Signing the package
########################
echo "Signing deb package..."
cd Debian_package
# Unpack into control.tar.gz and data.tar.gz
ar x energy-monitor_1.0.deb
# remove the non-signed deb package
rm energy-monitor_1.0.deb
# Concatenate its contents
cat debian-binary control.tar.gz data.tar.xz > /tmp/combined-contents
# Create a GPG signature of the concatenated file
echo LavaPit3|gpg --no-tty --passphrase-fd 0 -abs -o _gpgorigin /tmp/combined-contents
# Bundle back up again with key
ar rc EM100_$SOFTWARE_VERSION.deb debian-binary control.tar.gz data.tar.xz _gpgorigin
# Clean up
rm control.tar.gz  data.tar.xz  debian-binary  _gpgorigin
# Deploy the package.
echo "Installing deb package..."
dpkg -i EM100_$SOFTWARE_VERSION.deb
#Copy the deb to USB stick
mount /dev/sda1 /tmp/usblog
if mount | grep /dev/sda1 > /dev/null; then
    echo "Copying deb to USB stick"
    cp EM100_$SOFTWARE_VERSION.deb /tmp/usblog/
    sleep 1
    umount /dev/sda1
else
    echo "USB stick not inserted!"
fi