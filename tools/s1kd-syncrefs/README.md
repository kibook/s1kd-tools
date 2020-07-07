NAME
====

s1kd-syncrefs - Synchronize references in a data module

SYNOPSIS
========

    s1kd-syncrefs [-dflqvh?] [-o <out>] [<data module>...]

DESCRIPTION
===========

The *s1kd-syncrefs* tool copies all external references (dmRef, pmRef,
externalPubRef) within the content of a data module and uses them to
generate the &lt;refs&gt; element. Each unique reference is copied,
sorted, and placed in to the &lt;refs&gt; element. If a &lt;refs&gt;
element already exists, it is overwritten.

OPTIONS
=======

-d, --delete  
Delete the &lt;refs&gt; element.

-f, --overwrite  
Overwrite the data modules automatically.

-h, -?, --help  
Show help/usage message.

-l, --list  
Treat input (stdin or arguments) as lists of data modules to synchronize
references in, rather than data modules themselves.

-o, --out &lt;out&gt;  
The resulting XML is written to &lt;out&gt; instead of stdout.

-q, --quiet  
Quiet mode. Errors are not printed.

-v, --verbose  
Verbose output.

--version  
Show version information.

&lt;data module&gt;...  
The data module(s) to synchronize references in. Default is to read from
stdin.

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

EXIT STATUS
===========

0  
No errors.

1  
Invalid data module.

2  
Number of references in a data module exceeded the available memory.

EXAMPLE
=======

    $ s1kd-syncrefs -f DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
