NAME
====

s1kd-addicn - Add entity/notation declarations for an ICN

SYNOPSIS
========

s1kd-addicn \[-s &lt;src&gt;\] \[-o &lt;out&gt;\] \[-fh?\] &lt;ICN&gt;...

DESCRIPTION
===========

The *s1kd-addicn* tool adds the required DTD entity and notation declarations to an S1000D module in order to reference an ICN file.

OPTIONS
=======

-s &lt;src&gt;  
The source module to add the ICN to. Default is to read from stdin.

-o &lt;out&gt;  
The filename to output to. Default is to write to stdout.

-f  
Overwrite source file instead of writing to stdout.

-F  
Use the whole path given for the ICN file as the SYSTEM ID.

-h -?  
Show help/usage message.

&lt;ICN&gt;..  
Any number of ICN files to add.
