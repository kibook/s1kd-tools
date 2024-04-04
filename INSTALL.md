# General

There are multiple ways to install the s1kd-tools:

  - using a package manager and the pre-compiled Debian (.deb) or Red
    Hat (.rpm) packages

  - building from source

# Using a package manager

Debian (.deb) and Red Hat (.rpm) packages are provided to easily
install, upgrade or uninstall the s1kd-tools on Linux systems using a
package manager. The examples below focus on the standard `dpkg` (for
Debian-based distributions) and `rpm` (for Red Hat-based distributions).

## Installing

You can download the latest release of the s1kd-tools from
<http://khzae.net/1/s1000d/s1kd-tools/releases/latest>. Then use one of
the following commands to install it:

**Debian:**

    # dpkg -i s1kd-tools_[version]_[arch].deb

**Red Hat:**

    # rpm -i s1kd-tools.[version].[arch].rpm

## Uninstalling

To uninstall using the package manager, use one of the following
commands:

**Debian:**

    # dpkg -r s1kd-tools

**Red Hat:**

    # rpm -e s1kd-tools

# Building from source

## Requirements

The typical requirements to build the executables on any Linux
distribution are:

  - gcc

  - make

  - xxd

  - pkg-config

  - [libxml2](https://gitlab.gnome.org/GNOME/libxml2)

  - [libxslt](https://gitlab.gnome.org/GNOME/libxslt)

  - **If using the `SAXON` XPath engine:**
    [Saxon/C](https://www.saxonica.com/saxon-c/index.xml)

  - **If using the `XQILLA` XPath engine:**
    [Xerces-C](https://xerces.apache.org/xerces-c/),
    [XQilla](http://xqilla.sourceforge.net/HomePage)

To build the documentation from source:

  - [s1kd2db](http://github.com/kibook/s1kd2db)

  - [pandoc](https://pandoc.org/)

## Windows build environment

To build the executables on Windows, an environment such as MinGW or
Cygwin is recommended. These provide POSIX-compatible tools, such as
`make`, that allow the s1kd-tools to be built and installed on a Windows
system in the same way as on a Linux system.

### Cygwin

To build the executables on Cygwin, you will need the following packages
from the package installer:

  - gcc-core

  - make

  - xxd

  - libxml2-devel

  - libxslt-devel

### MSYS2 MINGW32

Prior to building the executables on MSYS2 MINGW32, run the following to
install the necessary packages from pacman:

    pacman -S mingw-w64-i686-gcc mingw-w64-i686-make mingw-w64-i686-pkgconf mingw-w64-i686-libxml2 mingw-w64-i686-libxslt mingw-w64-i686-libsystre vim

### MSYS2 MINGW64

Prior to building the executables on MSYS2 MINGW64, run the following to
install the necessary packages from pacman:

    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make mingw-w64-x86_64-pkgconf mingw-w64-x86_64-libxml2 mingw-w64-x86_64-libxslt mingw-w64-x86_64-libsystre vim

## Building and installing

Run the following commands to build the executables, and install both
the executables and documentation:

    $ make
    # make install

## Uninstalling

To uninstall the executables and documentation:

    # make uninstall

## Additional Makefile parameters

The following parameters can be given to `make` to control certain
options when building and installing.

### `prefix`

The `prefix` variable determines where the s1kd-tools are installed when
running `make install`, and where they are uninstalled from when running
`make uninstall`. The default value is `/usr/local`.

Example:

    # make prefix=/usr install

    # make prefix=/usr uninstall

### `xpath2_engine`

The `xpath2_engine` variable determines which XPath 2.0 implementation
the s1kd-brexcheck tool will use to evaluate the object paths of BREX
rules.

The s1kd-tools are built on libxml, so by default s1kd-brexcheck uses
libxml's XPath implementation. However, libxml only supports XPath 1.0.
While as of Issue 5.0, the S1000D default BREX rules are all compatible
with XPath 1.0, Issue 4.0 and up do reference the XPath 2.0
specification. Therefore, if your project needs XPath 2.0 support for
BREX rules, you should select one of these implementations:

  - `saxon`  
    Experimental implementation using the Saxon/C library. Slower, and
    Saxon/C itself is a very large dependency. Not recommended at this
    time due to memory leak issues.

  - `xqilla`  
    Experimental implementation using the Xerces-C and XQilla libraries.
    A little slower than libxml, but faster than Saxon/C, and the
    dependencies are much smaller than the latter. This is currently the
    recommended implementation if you need XPath 2.0 support.

Example:

    $ make xpath2_engine=xqilla

### `debug`

The `debug` variable enables building the tools with debugging
information still included.

Example:

    $ make debug=1
