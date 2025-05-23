#!/bin/sh

# Build .tar.gz packages for the s1kd-tools

set -e

if test $# != 1
then
	echo "Usage: $0 <version>"
	exit 1
fi

version=$1

build_tar() {
	xpath2_engine=$1

	name=s1kd-tools
	case "$xpath2_engine" in
		xqilla)
			name="$name+xpath2-xqilla"
			;;
	esac

	tmp=$(mktemp -d)
	
	make prefix="$tmp" clean all install

	tar -C "$tmp" -zcvf ${name}_${version}_linux-amd64.tar.gz bin share

	rm -rf "$tmp"
}

build_tar
build_tar xqilla
