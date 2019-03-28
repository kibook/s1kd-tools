NAME
====

s1kd-ls - List CSDB objects in a directory hierarchy

SYNOPSIS
========

    s1kd-ls [-0CDGIiLlMNoPRrwX] [<object>|<dir> ...]

DESCRIPTION
===========

The *s1kd-ls* tool searches the current directory or specified directory
trees and lists the file names of CSDB objects matching certain
criteria.

The files representing the CSDB objects must use either the standard
S1000D file naming conventions, or the alternate naming convention
supported by these tools using the -N option.

OPTIONS
=======

-0  
Output a null-delimited list of CSDB object paths.

-C, -D, -G, -L, -M, -P, -X  
List comments, data modules, ICNs, data management lists, ICN metadata
files, publication modules, and data dispatch notes respectively. If
none are specified, -CDGLMPX is assumed.

-h -?  
Show the usage message.

-I  
Show only inwork issues of objects (inwork != 00).

-i  
Show only official issues of objects (inwork = 00).

-l  
Show only the latest official/inwork issue of objects.

-N  
Assume that the files being listed do not include the issue info in
their filenames, i.e. they were created using the -N option of the
s1kd-new\* tools.

-o  
Show only old official/inwork issues of objects.

-R  
Show only non-writable object files.

-r  
Recursively descend in to directories.

-w  
Show only writable object files.

--version  
Show version information.

&lt;object&gt;\|&lt;dir&gt; ...  
An optional list of CSDB objects to list or directories to search for
CSDB objects in. If none are specified, CSDB objects in the current
directory are listed by default.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

EXAMPLE
=======

    $ s1kd-ls
    DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-001-01.JPG
    ICN-12345-00001-002-01.JPG
    PMC-EX-12345-00001-00_000-01_EN-CA.XML

    $ s1kd-ls -l
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-002-01.JPG
    PMC-EX-12345-00001-00_000-01_EN-CA.XML

    $ s1kd-ls -o
    DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-001-01.JPG

    $ s1kd-ls -D | s1kd-metadata -lt -ntechName -ninfoName -nissueDate
    Example A    Description    2018-03-20
    Example A    Description    2018-03-29
    Example B    Description    2018-03-29
