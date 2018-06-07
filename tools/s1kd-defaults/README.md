NAME
====

s1kd-defaults - `.defaults` and `.dmtypes` files management tool

SYNOPSIS
========

    s1kd-defaults [-Ddfisth?] [<file>...]

DESCRIPTION
===========

The *s1kd-defaults* tool generates a basic `.defaults` file for a new CSDB, which is used by several of the other s1kd-tools to determine default values for S1000D metadata. It also provides a way to convert between the simple text and XML formats of the `.defaults` and `.dmtypes` files.

OPTIONS
=======

-D  
Convert a `.dmtypes` file.

-d  
Convert a `.defaults` file.

-f  
Overwrite the existing file after conversion.

-i  
Initialize a new CSDB by generating both the `.defaults` and `.dmtypes` files in the current directory.

-s  
Sort the entries alphabetically for either file/output format.

-t  
Output using the simple text format. Otherwise, the XML format is used by default.

-h -?  
Show help/usage message.

--version  
Show version information.

&lt;file&gt;...  
Names of files to convert. If none are specified, the default names of `.defaults` (for the -d option) or `.dmtypes` (for the -D option) in the current directory are used.

EXAMPLES
========

Initialize a new CSDB, using the XML format
-------------------------------------------

    $ mkdir mycsdb
    $ cd mycsdb
    $ s1kd-defaults -i

Initialize a new CSDB, using the simple text format
---------------------------------------------------

    $ mkdir mycsdb
    $ cd mycsdb
    $ s1kd-defaults -ti

Generate a custom-named `.defaults` file
----------------------------------------

    $ s1kd-defaults > custom-defaults.xml

Convert a simple text formatted file to XML
-------------------------------------------

    $ s1kd-defaults -df

Sort entries and output in text format
--------------------------------------

    $ s1kd-defaults -dts custom-defaults.txt
