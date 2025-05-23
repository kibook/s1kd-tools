#!/bin/sh

set -e

out=build/$MSYSTEM

mkdir -p "$out"
make -j$(nproc) clean
make -j$(nproc) all
cp tools/*/*.exe "$out"
ldd "$out"/*.exe | awk '{print $3}' | grep '^/mingw' | sort -u | xargs -r cp -t "$out"
