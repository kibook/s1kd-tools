#!/bin/sh

# Build .deb packages for the s1kd-tools

set -e

if test $# != 1
then
	echo "Usage: $0 <version>"
	exit 1
fi

version=$1

build_deb() {
	xpath2_engine=$1

	name=s1kd-tools
	depends="libxml2, libxslt1.1"
	case "$xpath2_engine" in
		xqilla)
			name="$name+xqilla"
			depends="$depends, libxqilla6v5"
			;;
	esac

	tmp=$(pwd)/${name}_${version}
	mkdir "$tmp"
	
	make prefix="$tmp" clean all install

	mkdir "$tmp"/DEBIAN
	cat <<-EOF >"$tmp"/DEBIAN/control
	Package: s1kd-tools
	Version: $version
	Maintainer: khzae.net
	Architecture: amd64
	Depends: $depends
	Description: A set of small tools for manipulating S1000D data
	EOF

	dpkg-deb --build "$tmp"

	rm -rf "$tmp"
}

build_deb
build_deb xqilla
