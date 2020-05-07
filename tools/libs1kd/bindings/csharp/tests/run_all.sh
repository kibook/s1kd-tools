#!/bin/sh

set -e

run_test() {
	cd "$1"
	make
	LD_LIBRARY_PATH=../../../.. mono Test.exe
	cd ..
}

make -C ../../.. all

run_test instance
run_test brexcheck
run_test metadata
