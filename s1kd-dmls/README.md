NAME
====

s1kd-dmls - List data modules

SYNOPSIS
========

    s1kd-dmls [-acfHhilorTtpDP]

DESCRIPTION
===========

The *s1kd-dmls* tool lists data modules in a directory, with various options for columns for data module metadata which can be useful for sorting them with other tools.

OPTIONS
=======

-a  
Include the applicability column.

-c  
Show data module code column.

-D, -P, -C, -M  
List data modules, publication modules, comments and ICN metadata files respectively. If none are specified, -DPCM is assumed.

-f  
Do not show filename column.

-H  
Show headers on columns.

-h -?  
Show the usage message.

-I  
Show only official issues of data modules (inwork = 00).

-i  
Include the issue date column.

-L  
Show language info (languageIsoCode-countryIsoCode).

-l  
Show only the latest issue/inwork version of data modules.

-n  
Show issue info (issueNumber-inWork).

-o  
Include the originator column.

-p  
Do not replace control characters (\\n, \\t) when printing.

-R  
Recursively descend in to directories.

-r  
Include the responsible partner company column.

-T  
Show title in single column (techName - infoName).

-t  
Show tech and info name columns.

-w  
Show only writable data module files.
