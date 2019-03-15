NAME
====

s1kd-refs - Manage references between CSDB objects

SYNOPSIS
========

    s1kd-refs [-aCcDEFfGIilNnoPqRrsUuvXxh?] [-d <dir>] [<object>...]

DESCRIPTION
===========

The *s1kd-refs* tool lists external references in CSDB objects,
optionally matching them to a filename in the CSDB directory hierarchy.

This allows you to:

-   obtain a list of dependencies for CSDB objects, such as ICNs, to
    ensure they are delivered together

-   check for references to objects which do not exist in the current
    CSDB

-   update reference metadata, such as titles, from the matched objects

OPTIONS
=======

-a  
List all references, both matched and unmatched.

-C, -D, -E, -G, -P  
List references to comments, data modules, external publications, ICNs,
and publication modules respectively. If none are specified, -CDEGP is
assumed.

-c  
List references in the `content` section of a CSDB object only.

-d &lt;dir&gt;  
Directory to search for matches to references in. By default, the
current directory is used.

-F  
When using the -U or -X options, overwrite the input objects that have
been updated or tagged.

-f  
Include the filename of the source object where each reference was found
in the output.

-h -?  
Show help/usage message.

-I  
Update the issue number, issue date, language, and title of references
to that of the latest matched object. This option implies the -U and -i
options.

-i  
Always match the latest issue of an object found, regardless of the
issue and language specified in the reference.

-l  
Treat input (stdin or arguments) as lists of filenames of CSDB objects
to list references in, rather than CSDB objects themselves.

-N  
Assume filenames of referenced CSDB objects omit the issue info, i.e.
they were created with the -N option to the s1kd-new\* tools.

-n  
Include the filename of the source object where each reference was
found, and display the line number where the reference occurs in the
source file after its filename.

-o  
Output valid CSDB objects to stdout.

-q  
Quiet mode. Errors are not printed.

-R  
List references in matched objects recursively.

-r  
Search for matches to references in directories recursively.

-s  
Include the source object as a reference. This is helpful when the
output of this tool is used to apply some operation to a source object
and all its dependencies together.

-U  
Update the title of matched references from the corresponding object.

-u  
Show only unmatched reference errors, or unmatched codes if combined
with the -a option.

-v  
Verbose output.

-X  
Tag unmatched references with the processing instruction
`<?unmatched?>`.

-x  
Output a detailed XML report instead of plain text messages.

--version  
Show version information.

&lt;object&gt;...  
CSDB object(s) to list references in. If none are specified, the tool
will read from stdin.

EXIT STATUS
===========

0  
No errors, all references were matched.

1  
Some references were unmatched.

2  
The number of objects found in a recursive check (-R) exceeded the
available memory.

3  
stdin did not contain valid XML and not in list mode (-l).

EXAMPLE
=======

    $ s1kd-refs DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-EX-A-00-00-00-00A-022A-D_001-00_EN-CA.XML
    DMC-EX-A-01-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-001-01.JPG
