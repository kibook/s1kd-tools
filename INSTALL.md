Requirements
============

To build the executables:

-   coreutils and binutils

-   xxd

-   libxml2, libxslt, libexslt

To build the documentation from source:

-   s1kd2db XSLT script

-   pandoc

Building the executables
========================

Run the following commands to build the executables, and install both the executables and documentation:

    $ make
    # make install

To change where these are installed, specify either of the following make variables:

INSTALL\_PREFIX  
Prefix where the executables will be installed.

Default: /usr/bin

MAN\_PREFIX  
Prefix where the manpages will be installed.

Default: /usr/local/share/man/man1

For example:

    $ make INSTALL_PREFIX=/usr/local/bin install
