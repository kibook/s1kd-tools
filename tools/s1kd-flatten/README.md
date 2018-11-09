NAME
====

s1kd-flatten - Flatten a publication module for publishing

SYNOPSIS
========

    s1kd-flatten [-d <dir>] [-I <path>] [-cDfNpRrx] <PM> [<DM>...]

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

-c  
Flatten referenced container data modules by copying the references
inside the container directly in to the publication module. The copied
references will also be flattened, unless the -d option is specified.

-D  
Remove unresolved references, but do not flatten resolved ones.

-d &lt;dir&gt;  
Directory to start search in. By default, the current directory is used.

-f  
Overwrite input publication module instead of writing to stdout.

-h -?  
Show help/usage message.

-I &lt;path&gt;  
Add &lt;path&gt; to the list of directories that the tool will search
when resolving references.

-N  
Assume that the files representing the referenced data modules do not
include the issue info in their filenames, i.e. they were created using
the -N option of the s1kd-new\* tools.

-p  
Instead of the hierarchical PM-based format, use a simpler "flat"
format.

-R  
Recursively flatten referenced publication modules, copying their
content in to the "master" publication module.

-r  
Search directories recursively.

-x  
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

EXAMPLE
=======

    $ s1kd-flatten -x PMC-EX-12345-00001-00_001-00_EN-CA.XML > Book.xml
