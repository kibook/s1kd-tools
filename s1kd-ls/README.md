NAME
====

s1kd-ls - List CSDB objects

SYNOPSIS
========

    s1kd-ls [-0CDIiLlMoPrwX]

DESCRIPTION
===========

The *s1kd-ls* tool lists CSDB objects in a directory hierarchy.

OPTIONS
=======

-0  
Output a null-delimited list of CSDB object paths.

-C, -D, -L, -M, -P, -X  
List comments, data modules, data management lists, ICN metadata files, publication modules, and data dispatch notes respectively. If none are specified, -CDLMPX is assumed.

-h -?  
Show the usage message.

-I  
Show only inwork issues of objects (inwork != 00).

-i  
Show only official issues of objects (inwork = 00).

-l  
Show only the latest official/inwork issue of objects.

-o  
Show only old official/inwork issues of objects.

-r  
Recursively descend in to directories.

-w  
Show only writable object files.

EXAMPLE
=======

    $ s1kd-ls
    DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML
    PMC-EX-12345-00001-00_000-01_EN-CA.XML

    $ s1kd-ls -l
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML
    PMC-EX-12345-00001-00_000-01_EN-CA.XML

    $ s1kd-ls -o
    DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML

    $ s1kd-ls -D | s1kd-metadata -lt -ntechName -ninfoName -nissueDate
    Example A    Description    2018-03-20
    Example A    Description    2018-03-29
    Example B    Description    2018-03-29
