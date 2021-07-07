NAME
====

s1kd-flatten - Flatten a publication module for publishing

SYNOPSIS
========

    s1kd-flatten [-d <dir>] [-I <path>] [-cDfimNPpqRruvx] <PM> [<DM>...]

DESCRIPTION
===========

The *s1kd-flatten* tool combines a publication module and the data
modules it references in to a single file for use with a publishing
system.

Data modules are by default searched for in the current directory using
the data module code, language and/or issue info provided in each
reference.

OPTIONS
=======

-c, --containers  
Flatten referenced container data modules by copying the references
inside the container directly in to the publication module. The copied
references will also be flattened, unless the -m option is specified.

-D, --remove  
Remove unresolved references.

-d, --dir &lt;dir&gt;  
Directory to start search in. By default, the current directory is used.

-f, --overwrite  
Overwrite input publication module instead of writing to stdout.

-h, -?, --help  
Show help/usage message.

-I, --include &lt;path&gt;  
Add &lt;path&gt; to the list of directories that the tool will search
when resolving references.

-i, --ignore-issue  
Always match the latest issue of an object found, regardless of the
issue specified in the reference.

-l, --list  
Treat input (stdin or arguments) as lists of CSDB objects, rather than
CSDB objects themselves. This option only applies to the simple "flat"
format (-p/--simple).

-m, --modify  
Modify the references in the publication module without flattening them.

-N, --omit-issue  
Assume that the files representing the referenced data modules do not
include the issue info in their filenames, i.e. they were created using
the -N option of the s1kd-new\* tools.

-P, --only-pm-refs  
Only flatten PM references, leaving DM references alone.

-p, --simple  
Instead of the hierarchical PM-based format, use a simpler "flat"
format.

-q, --quiet  
Quiet mode. Errors are not printed.

-R, --recursively  
Recursively flatten referenced publication modules, copying their
content in to the "master" publication module.

-r, --recursive  
Search directories recursively.

-u, --unique  
Remove duplicate references within the PM content.

-v, --verbose  
Verbose output. Specify multiple times to increase the verbosity.

-x, --use-xinclude  
Use XInclude rather than copying each data module's contents directly
inside the publication module. DTD entities in data modules will only be
carried over to the final publication when using this option, otherwise
they do not carry over when copying the data module.

--version  
Show version information.

&lt;DM&gt;...  
When using the -p option, the filenames to include can be specified
manually as additional arguments instead of searching for them in the
current directory. When not using the -p option, additional arguments
are ignored.

&lt;PM&gt;  
The publication module to flatten.

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

EXIT STATUS
===========

0  
No errors.

1  
The publication module specified is malformed.

2  
An encoding error occurred.

EXAMPLE
=======

    $ s1kd-flatten -x PMC-EX-12345-00001-00_001-00_EN-CA.XML > Book.xml
