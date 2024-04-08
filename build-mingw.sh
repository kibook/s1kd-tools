#!/bin/sh

set -e

out=build/$MSYSTEM

for module in gcc make pkgconf libxml2 libxslt libsystre vim
do
	pacman --noconfirm -S --needed ${MINGW_PACKAGE_PREFIX}-${module}
done

mkdir -p "$out"
make -j$(nproc)
cp tools/*/*.exe "$out"
ldd "$out"/*.exe | awk '{print $3}' | grep '^/mingw' | sort -u | xargs cp -t "$out"
