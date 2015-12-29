#!/bin/sh
# Script to download squashfs-tools v4.3, apply the patches, perform a clean build, and install.

if [ "$UID" == "0" ]
then
    SUDO=""
else
    SUDO="sudo"
fi

if [ ! -e squashfs4.3.tar.gz ]
then
    wget http://downloads.sourceforge.net/project/squashfs/squashfs/squashfs4.3/squashfs4.3.tar.gz
fi

rm -rf squashfs4.3
tar -zxvf squashfs4.3.tar.gz
cd squashfs4.3
patch -p0 < ../patches/patch0.txt
cd squashfs-tools
make && $SUDO make install
