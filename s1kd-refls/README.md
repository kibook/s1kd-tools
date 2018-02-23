NAME
====

s1kd-refls - List references in a CSDB object

SYNOPSIS
========

    s1kd-refls [-qcaNh?] <objects>...

DESCRIPTION
===========

The *s1kd-refls* tool lists external references to other CSDB objects (dmRef, pmRef), optionally matching them to a filename in the current directory. This makes it easy to see what a given CSDB object "depends" on.

OPTIONS
=======

-a  
List all references, not attempting to match them to an actual filename.

-c  
List references in the `content` section of a CSDB object only.

-h -?  
Show help/usage message.

-N  
Assume filenames of referenced CSDB objects omit the issue info, i.e. they were created with the -N option to the s1kd-new\* tools.

-q  
Quiet mode. Errors are not printed.
