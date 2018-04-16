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

To change where these are installed, specify the PREFIX make variable. The default value of this variable is /usr/local.

For example:

    $ make PREFIX=/usr install
