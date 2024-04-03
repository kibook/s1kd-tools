#!/bin/sh

out=build/mingw64

make
mkdir -p "$out"
cp tools/s1kd-*/s1kd-*.exe "$out"
ldd "$out"/s1kd-*.exe | awk '{print $3}' | grep '^/mingw64' | sort -u | xargs cp -t "$out"
