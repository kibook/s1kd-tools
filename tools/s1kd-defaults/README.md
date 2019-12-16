NAME
====

s1kd-defaults - `.defaults`, `.dmtypes` and `.fmtypes` files management
tool

SYNOPSIS
========

    s1kd-defaults [-DdFfisth?] [-b <BREX>] [-j <map>]
                  [-n <name> -v <value> ...] [-o <dir>] [<file>...]

DESCRIPTION
===========

The *s1kd-defaults* tool generates a basic `.defaults` file for a new
CSDB, which is used by several of the other s1kd-tools to determine
default values for S1000D metadata. It also provides a way to convert
between the simple text and XML formats of the `.defaults`, `.dmtypes`
and `.fmtypes` files.

OPTIONS
=======

-b, --brex &lt;BREX&gt;  
Use the specified BREX data module to build the `.defaults` and
`.dmtypes` files. This can be used both when initializing a new CSDB
(-i) or either file can be generated from a BREX data module separately.

-D, --dmtypes  
Convert a `.dmtypes` file.

-d, --defaults  
Convert a `.defaults` file.

-F, --fmtypes  
Convert a `.fmtypes` file.

-f, --overwrite  
Overwrite the existing file after conversion.

-h, -?, --help  
Show help/usage message.

-i, --init  
Initialize a new CSDB by generating the `.defaults`, `.dmtypes` and
`.fmtypes` files in the current directory.

-J, --dump-brexmap  
Dump the default `.brexmap` file to stdout.

-j, --brexmap &lt;map&gt;  
Use a custom `.brexmap` file to map a BREX DM to a `.defaults` or
`.dmtypes` file.

-n, --name &lt;name&gt;  
The name of a specific default key to set a value for. The value must be
specified after this option with -v. Multiple pairs of -n and -v can be
specified to set multiple default values.

-o, --dir &lt;dir&gt;  
Initialize or manage configuration files in &lt;dir&gt; instead of the
current directory. If &lt;dir&gt; does not exist, it will be created.

-s, --sort  
Sort the entries alphabetically for either file/output format.

-t, --text  
Output using the simple text format. Otherwise, the XML format is used
by default.

-v, --value &lt;value&gt;  
The new value to set for the default key specified with -n. This option
must be specified after -n.

--version  
Show version information.

&lt;file&gt;...  
Names of files to convert. If none are specified, the default names of
`.defaults` (for the -d option), `.dmtypes` (for the -D option) or
`.fmtypes` (for the -F option) in the current directory are used.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--xinclude  
Do XInclude processing.

`.brexmap` file
---------------

This file specifies a mapping between BREX structure object rules and
`.defaults` and `.dmtypes` files. The path to an object can be written
in many different ways in a BREX rule, so the `.brexmap` file allows any
project's BREX to be used to generate these files without having to
modify the BREX data module itself.

By default, the program will search for a file named `.brexmap` in the
current directory and parent directories, but any file can be specified
using the -j option. If there is no `.brexmap` file and the -j option is
not specified, a default mapping will be used.

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

Set a default value in the current `.defaults` file
---------------------------------------------------

    $ s1kd-defaults -df -n issue -v 5.0
