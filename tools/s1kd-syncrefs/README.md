NAME
====

s1kd-syncrefs - Synchronize references in a data module

SYNOPSIS
========

`s1kd-syncrefs [-dfl] [-o <out>] [<data module>...]`

DESCRIPTION
===========

The *s1kd-syncrefs* tool copies all external references (dmRef, pmRef, externalPubRef) within the content of a data module and uses them to generate the &lt;refs&gt; element. Each unique reference is copied, sorted, and placed in to the &lt;refs&gt; element. If a &lt;refs&gt; element already exists, it is overwritten.

OPTIONS
=======

-d  
Delete the &lt;refs&gt; element.

-f  
Overwrite the data modules automatically.

-l  
Treat input (stdin or arguments) as lists of data modules to synchronize references in, rather than data modules themselves.

-o &lt;out&gt;  
The resulting XML is written to &lt;out&gt; instead of stdout.

--version  
Show version information.

&lt;data module&gt;...  
The data module(s) to synchronize references in. Default is to read from stdin.
