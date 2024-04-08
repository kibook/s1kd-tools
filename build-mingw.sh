#!/bin/sh

set -e

out=build/$MSYSTEM

pacman --noconfirm -S --needed $(printf "${MINGW_PACKAGE_PREFIX}-%s " gcc make pkgconf libxml2 libxslt libsystre) vim

mkdir -p "$out"
make -j$(nproc)
cp tools/*/*.exe "$out"
ldd "$out"/*.exe | awk '{print $3}' | grep '^/mingw' | sort -u | xargs -r cp -t "$out"
