#!/bin/sh

set -e

make -C ../../.. all
make -C .. clean all
make clean all

LD_LIBRARY_PATH=../../.. ./tests
