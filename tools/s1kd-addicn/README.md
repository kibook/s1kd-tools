NAME
====

s1kd-addicn - Add entity/notation declarations for an ICN

SYNOPSIS
========

    s1kd-addicn [-o <file>] [-s <src>] [-fh?] <ICN>...

DESCRIPTION
===========

The *s1kd-addicn* tool adds the required DTD entity and notation
declarations to an S1000D module in order to reference an ICN file.

OPTIONS
=======

-F, --full-path  
Use the whole path given for the ICN file as the SYSTEM ID.

-f, --overwrite  
Overwrite source file instead of writing to stdout.

-h, -?, --help  
Show help/usage message.

-o, --out &lt;out&gt;  
The filename to output to. Default is to write to stdout.

-s, --source &lt;src&gt;  
The source module to add the ICN(s) to. Default is to read from stdin.

--version  
Show version information.

&lt;ICN&gt;..  
Any number of ICN files to add.

In addition, the following options allow configuration of the XML
parser:

--dtdload  
Load the external DTD.

--huge  
Remove any internal arbitrary parser limits.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--parser-errors  
Emit errors from parser.

--parser-warnings  
Emit warnings from parser.

--xinclude  
Do XInclude processing.

--xml-catalog &lt;file&gt;  
Use an XML catalog when resolving entities. Multiple catalogs may be
loaded by specifying this option multiple times.

EXAMPLE
=======

    $ s1kd-addicn -fs <DM> ICN-EX-12345-001-01.JPG
