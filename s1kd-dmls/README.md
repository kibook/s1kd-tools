NAME
====

s1kd-dmls - List data modules

SYNOPSIS
========

s1kd-dmls \[-acfHhilorTt\]

DESCRIPTION
===========

The *s1kd-dmls* tool lists data modules in a directory, with various options for columns for data module metadata which can be useful for sorting them with other tools.

OPTIONS
=======

-l  
Show only the latest issue/inwork version of data modules.

-I  
Show only official issues of data modules (inwork = 00).

-f  
Do not show filename column.

-c  
Show data module code column.

-n  
Show issue info (issueNumber-inWork).

-L  
Show language info (languageIsoCode-countryIsoCode).

-t  
Show tech and info name columns.

-T  
Show title in single column (techName - infoName).

-i  
Include the issue date column.

-r  
Include the responsible partner company column.

-o  
Include the originator column.

-a  
Include the applicability column.

-H  
Show headers on columns.

-w  
Show only writable data module files.

-h  
Show the usage message.
