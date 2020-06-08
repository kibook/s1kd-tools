General
=======

There are multiple ways to install the s1kd-tools:

-   using a package manager and the pre-compiled Debian (.deb) or Red
    Hat (.rpm) packages

-   building from source

Using a package manager
=======================

Debian (.deb) and Red Hat (.rpm) packages are provided to easily
install, upgrade or uninstall the s1kd-tools on Linux systems using a
package manager. The examples below focus on the standard `dpkg` (for
Debian-based distributions) and `rpm` (for Red Hat-based distributions).

Installing
----------

You can download the latest release of the s1kd-tools from
<http://khzae.net/1/s1000d/s1kd-tools/releases/latest>. Then use one of
the following commands to install it:

**Debian:**

    # dpkg -i s1kd-tools_[version]_[arch].deb

**Red Hat:**

    # rpm -i s1kd-tools.[version].[arch].rpm

Uninstalling
------------

To uninstall using the package manager, use one of the following
commands:

**Debian:**

    # dpkg -r s1kd-tools

**Red Hat:**

    # rpm -e s1kd-tools

Building from source
====================

Requirements
------------

To build the executables:

-   coreutils and binutils

-   xxd

-   [libxml2, libxslt, libexslt](http://xmlsoft.org)

To build the documentation from source:

-   [s1kd2db](http://github.com/kibook/s1kd2db)

-   [pandoc](https://pandoc.org/)

Building and installing
-----------------------

Run the following commands to build the executables, and install both
the executables and documentation:

    $ make
    # make install

To change where these are installed, specify the PREFIX make variable.
The default value of this variable is /usr/local.

For example:

    # make PREFIX=/usr install

Uninstalling
------------

To uninstall the executables and documentation:

    # make uninstall

Remember to specify the PREFIX make variable if a different prefix was
used during installation.
