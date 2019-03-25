NAME
====

s1kd-addicn - Add entity/notation declarations for an ICN

SYNOPSIS
========

    s1kd-addicn [-s <src>] [-o <out>] [-fh?] <ICN>...

DESCRIPTION
===========

The *s1kd-addicn* tool adds the required DTD entity and notation
declarations to an S1000D module in order to reference an ICN file.

OPTIONS
=======

-F  
Use the whole path given for the ICN file as the SYSTEM ID.

-f  
Overwrite source file instead of writing to stdout.

-h -?  
Show help/usage message.

-o &lt;out&gt;  
The filename to output to. Default is to write to stdout.

-s &lt;src&gt;  
The source module to add the ICN(s) to. Default is to read from stdin.

--version  
Show version information.

&lt;ICN&gt;..  
Any number of ICN files to add.

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

    $ s1kd-addicn -fs <DM> ICN-EX-12345-001-01.JPG
