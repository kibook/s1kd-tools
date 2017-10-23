NAME
====

s1kd-newpm - Create new S1000D publication module.

SYNOPSIS
========

s1kd-newpm \[options\]

DESCRIPTION
===========

The *s1kd-newpm* tool creates a new S1000D publication module with the publication module code and other metadata specified.

OPTIONS
=======

-d  
Specify the 'defaults' file name.

-p  
Prompt the user for any values left unspecified.

-\# &lt;PMC&gt;  
The publication module code of the new publication module.

-L &lt;language&gt;  
The language ISO code of the new publication module.

-C &lt;country&gt;  
The country ISO code of the new publication module.

-n &lt;issue&gt;  
The issue number of the new publication module.

-w &lt;inwork&gt;  
The inwork number of the new publication module.

-c &lt;sec&gt;  
The security classification of the new publication module.

-r &lt;RPC&gt;  
The responsible partner company enterprise name of the new publication module.

-R &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-t &lt;title&gt;  
The title of the new publication module.

-b &lt;BREX&gt;  
BREX data module code.

-I &lt;date&gt;  
The issue date of the new publication module in the form of YYYY-MM-DD.

-v  
Print the file name of the newly created publication module.

-f  
Overwrite existing file.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
