#!/bin/sh

make -C .. all
make all

LD_LIBRARY_PATH=.. valgrind --leak-check=full --error-exitcode=1 ./tests
