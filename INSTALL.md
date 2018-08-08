Requirements
============

To build the executables:

-   coreutils and binutils

-   xxd

-   libxml2, libxslt, libexslt

To build the documentation from source:

-   [s1kd2db](http://github.com/kibook/s1kd2db)

-   pandoc

Building the executables
========================

Run the following commands to build the executables, and install both the executables and documentation:

    $ make
    # make install

To change where these are installed, specify the PREFIX make variable. The default value of this variable is /usr/local.

For example:

    $ make PREFIX=/usr install

Uninstalling
============

To uninstall the executables and documentation:

    # make uninstall

Remember to specify the PREFIX make variable if a different prefix was used during installation.
