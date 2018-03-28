NAME
====

s1kd-ls - List CSDB objects

SYNOPSIS
========

    s1kd-ls [-CDiLlMPrwX]

DESCRIPTION
===========

The *s1kd-ls* tool lists CSDB objects in a directory hierarchy.

OPTIONS
=======

-C, -D, -L, -M, -P, -X  
List comments, data modules, data management lists, publication modules, and data dispatch notes respectively. If none are specified, -CDLMPX is assumed.

-h -?  
Show the usage message.

-i  
Show only official issues of data modules (inwork = 00).

-l  
Show only the latest issue/inwork version of data modules.

-r  
Recursively descend in to directories.

-w  
Show only writable data module files.
