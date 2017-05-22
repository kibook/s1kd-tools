NAME
====

s1kd-syncrefs - Synchronize references in a data module

SYNOPSIS
========

s1kd-syncrefs \[-o &lt;out&gt;\] &lt;datamodules&gt;

DESCRIPTION
===========

The *s1kd-syncrefs* tool copies all external references (dmRef, pmRef, externalPubRef) within the content of a data module and uses them to generate the &lt;refs&gt; element. Each unique reference is copied, sorted, and placed in to the &lt;refs&gt; element. If a &lt;refs&gt; element already exists, it is overwritten.

OPTIONS
=======

-o &lt;out&gt;  
The resulting data module is output to the file &lt;out&gt; instead of overwriting the original data module. This option only makes sense when &lt;datamodules&gt; contains only a single data module to synchronize. - can be specified to print to stdout.

&lt;datamodules&gt;  
The data modules to synchronize references in. Each data module will be overwritten as a result of this command.
