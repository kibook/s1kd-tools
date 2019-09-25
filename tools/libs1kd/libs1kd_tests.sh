#!/bin/sh

make libs1kd.so libs1kd_tests

LD_LIBRARY_PATH=. valgrind --leak-check=full ./libs1kd_tests
