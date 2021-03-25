#!/bin/bash
# Script to download squashfs-tools v4.3, apply the patches, perform a clean build, and install.

# If not root, perform 'make install' with sudo
if [ $UID -eq 0 ]
then
    SUDO=""
else
    SUDO="sudo"
fi

# Install prerequisites
if hash apt-get &>/dev/null
then
    $SUDO apt-get install build-essential liblzma-dev liblzo2-dev zlib1g-dev
fi

# Make sure we're working in the same directory as the build.sh script
cd $(dirname `readlink  -f $0`)

# Download squashfs4.3.tar.gz if it does not already exist
if [ ! -e squashfs4.3.tar.gz ]
then
    wget https://downloads.sourceforge.net/project/squashfs/squashfs/squashfs4.3/squashfs4.3.tar.gz
fi

# Remove any previous squashfs4.3 directory to ensure a clean patch/build
rm -rf squashfs4.3

# Extract squashfs4.3.tar.gz
tar -zxvf squashfs4.3.tar.gz

# Patch, build, and install the source
cd squashfs4.3
patch -p0 < ../patches/patch0.txt
cd squashfs-tools
make && $SUDO make install
