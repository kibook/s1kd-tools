NAME
====

s1kd-refls - List references in a CSDB object

SYNOPSIS
========

    s1kd-refls [-aCcDEfGilNnPqruh?] [-d <dir>] [<object>...]

DESCRIPTION
===========

The *s1kd-refls* tool lists external references to other CSDB objects (dmRef, pmRef), optionally matching them to a filename in the current directory. This makes it easy to see what a given CSDB object "depends" on.

OPTIONS
=======

-a  
List all references, not attempting to match them to an actual filename.

-C, -D, -E, -G, -P  
List references to comments, data modules, external publications, ICNs, and publication modules respectively. If none are specified, -CDEGP is assumed.

-c  
List references in the `content` section of a CSDB object only.

-d &lt;dir&gt;  
Directory to search for matches to references in. By default, the current directory is used.

-f  
Include the filename of the source object where each reference was found in the output.

-h -?  
Show help/usage message.

-i  
Ignore issue and language info when matching references.

-l  
Treat input (stdin or arguments) as lists of filenames of CSDB objects to list references in, rather than CSDB objects themselves.

-N  
Assume filenames of referenced CSDB objects omit the issue info, i.e. they were created with the -N option to the s1kd-new\* tools.

-n  
If the -f option is used, display the line number where the reference occurs in the source file after its filename.

-q  
Quiet mode. Errors are not printed.

-r  
Search for matches to references in directories recursively.

-u  
Show only unmatched reference errors, or unmatched codes if combined with the -a option.

--version  
Show version information.

&lt;object&gt;...  
CSDB object(s) to list references in. If none are specified, the tool will read from stdin.

EXAMPLE
=======

    $ s1kd-refls DMC-EX-A-00-00-00-00A-040A-D_EN-CA.XML
