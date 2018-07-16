NAME
====

s1kd-defaults - `.defaults`, `.dmtypes` and `.fmtypes` files management tool

SYNOPSIS
========

    s1kd-defaults [-DdFfisth?] [-b <BREX>] [-j <map>] [<file>...]

DESCRIPTION
===========

The *s1kd-defaults* tool generates a basic `.defaults` file for a new CSDB, which is used by several of the other s1kd-tools to determine default values for S1000D metadata. It also provides a way to convert between the simple text and XML formats of the `.defaults`, `.dmtypes` and `.fmtypes` files.

OPTIONS
=======

-b &lt;BREX&gt;  
Use the specified BREX data module to build the `.defaults` and `.dmtypes` files. This can be used both when initializing a new CSDB (-i) or either file can be generated from a BREX data module separately.

-D  
Convert a `.dmtypes` file.

-d  
Convert a `.defaults` file.

-F  
Convert a `.fmtypes` file.

-f  
Overwrite the existing file after conversion.

-i  
Initialize a new CSDB by generating the `.defaults`, `.dmtypes` and `.fmtypes` files in the current directory.

-j &lt;map&gt;  
Use a custom .brexmap file to map a BREX DM to a `.defaults` or `.dmtypes` file.

-s  
Sort the entries alphabetically for either file/output format.

-t  
Output using the simple text format. Otherwise, the XML format is used by default.

-h -?  
Show help/usage message.

--version  
Show version information.

&lt;file&gt;...  
Names of files to convert. If none are specified, the default names of `.defaults` (for the -d option), `.dmtypes` (for the -D option) or `.fmtypes` (for the -F option) in the current directory are used.

`.brexmap` file
---------------

This file specifies a mapping between BREX structure object rules and `.defaults` and `.dmtypes` files. The path to an object can be written in many different ways in a BREX rule, so the `.brexmap` file allows any project's BREX to be used to generate these files without having to modify the BREX data module itself.

By default, the program will search for a file named `.brexmap` in the current directory, but any file can be specified using the -j option. If there is no `.brexmap` file and the -j option is not specified, a default mapping will be used.

Example of `.brexmap` file:

    <brexMap>
    <dmtypes path="//@infoCode"/>
    <default path="//@languageIsoCode" ident="languageIsoCode"/>
    <default path="//@countryIsoCode" ident="countryIsoCode"/>
    </brexMap>

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
