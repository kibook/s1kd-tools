NAME
====

s1kd-syncrefs - Synchronize references in a data module

SYNOPSIS
========

    s1kd-syncrefs [-dflv] [-o <out>] [<data module>...]

DESCRIPTION
===========

The *s1kd-syncrefs* tool copies all external references (dmRef, pmRef,
externalPubRef) within the content of a data module and uses them to
generate the &lt;refs&gt; element. Each unique reference is copied,
sorted, and placed in to the &lt;refs&gt; element. If a &lt;refs&gt;
element already exists, it is overwritten.

OPTIONS
=======

-d  
Delete the &lt;refs&gt; element.

-f  
Overwrite the data modules automatically.

-l  
Treat input (stdin or arguments) as lists of data modules to synchronize
references in, rather than data modules themselves.

-o &lt;out&gt;  
The resulting XML is written to &lt;out&gt; instead of stdout.

-v  
Verbose output.

--version  
Show version information.

&lt;data module&gt;...  
The data module(s) to synchronize references in. Default is to read from
stdin.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

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
