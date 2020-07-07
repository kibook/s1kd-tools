NAME
====

s1kd-sns - Organize data modules based on an SNS

SYNOPSIS
========

    s1kd-sns [-D <dir>] [-d <dir>] [-cmnpsh?] [<BREX> ...]

DESCRIPTION
===========

The *s1kd-sns* tool can be used to automatically organize data modules
in a CSDB in to a directory hierarchy based on a specified SNS
structure. It may also be used to simply print an indented text version
of an SNS structure.

OPTIONS
=======

-c, --copy  
Copy files in to the SNS subfolders instead of linking them.

-D, --srcdir &lt;dir&gt;  
The flat directory containing the data modules to organize. By default,
the current directory is used.

-d, --outdir &lt;dir&gt;  
The root directory of the new SNS structure. By default, the tool will
use the name "SNS" in the current directory.

-h, -?, --help  
Show usage message.

-m, --move  
Move files in to the SNS subfolders instead of linking them.

-n, --only-code  
Use only the SNS codes when naming directories. By default, each
directory will be named in the form of "snsCode - snsTitle".

-p, --print  
Print the SNS structure only.

-s, --symlink  
Use symbolic links to organize the SNS instead of the default hard
links.

--version  
Show version information.

&lt;BREX&gt;  
Read the SNS structure from the specified BREX data module. If none is
specified, the tool will read from stdin.

In addition, the following options allow configuration of the XML
parser:

--dtdload  
Load the external DTD.

--huge  
Remove any internal arbitrary parser limits.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--parser-errors  
Emit errors from parser.

--parser-warnings  
Emit warnings from parser.

--xinclude  
Do XInclude processing.

--xml-catalog &lt;file&gt;  
Use an XML catalog when resolving entities. Multiple catalogs may be
loaded by specifying this option multiple times.

EXAMPLE
=======

    $ s1kd-sns DMC-S1000D-A-08-02-0100-00A-022A-D_EN-US.XML
    $ tree SNS
    SNS
    |_ 00 - Product, General
       |_ 0 - Product, General
       |_ 1 - Product, General maintenance
       |_ 2 - Product, Safety
       |
    ...
    |_ 04 - Worthiness (fit for purpose) limitations
       |_ 0 - General
       |_ 1 - Fatigue index calculations
       |_ 2 - Operating spectrums
    |_ 05 - Scheduled/unscheduled maintenance
       |_ 0 - General
       |_ 1 - Time limits
       |_ 2 - Scheduled maintenance check lists
    ...
    |_ 18 - Vibration and noise analysis and attenuation
