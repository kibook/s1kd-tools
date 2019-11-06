NAME
====

s1kd-repcheck - Validate CIR references in S1000D CSDB objects

SYNOPSIS
========

    s1kd-repcheck [options] [<objects>...]

DESCRIPTION
===========

The *s1kd-repcheck* tool validates references to Common Information
Repository (CIR) items within S1000D CSDB objects. Any CIR references
which cannot be resolved to a specification within a CIR data module
will cause the tool to report an error.

OPTIONS
=======

-a, --all  
In addition to CIR data modules specified with -R or explicitly linked
in CIR references, allow CIR references to be resolved against any CIR
data modules that were specified as objects to check.

-d, --dir &lt;dir&gt;  
The directory to start searching for CIR data modules in. By default,
the current directory is used.

-f, --filenames  
Print the filenames of invalid objects.

-h, -?, --help  
Show help/usage message.

-l, --list  
Treat input as a list of CSDB objects to check.

-N, --omit-issue  
Assume that the issue/inwork numbers are omitted from object filenames
(they were created with the -N option).

-o, --output-valid  
Output valid CSDB objects to stdout.

-p, --progress  
Display a progress bar.

-q, --quiet  
Quiet mode. Error messages will not be printed.

-R, --cir &lt;CIR&gt;  
A CIR to resolve references in CSDB objects against. Multiple CIRs can
be specified by using this option multiple times. If "\*" is given for
&lt;CIR&gt;, the tool will search for CIR data modules.

-r, --recursive  
Search for CIR data modules recursively.

-T, --summary  
Print a summary of the check after it completes, including statistics on
the number of objects that passed/failed the check.

-v, --verbose  
Verbose output. Specify multiple times to increase the verbosity.

-x, --xml  
Print an XML report of the check.

--version  
Show version information.

&lt;object&gt;...  
Object(s) to check CIR references in.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--xinclude  
Do XInclude processing.

EXIT STATUS
===========

0  
The check completed successfully, and all CIR references were resolved.

1  
The check completed successfully, but some CIR references could not be
resolved.

2  
The number of CSDB objects specified exceeded the available memory.

EXAMPLE
=======

Part repository:

    <partRepository>
    <partSpec>
    <partIdent manufacturerCodeValue="12345" partNumberValue="ABC"/>
    <itemIdentData>
    <descrForPart>ABC part</descrForPart>
    </itemIdentData>
    </partSpec>
    </partRepository>

Part references in a procedure:

    <spareDescrGroup>
    <spareDescr>
    <partRef manufacturerCodeValue="12345" partNumberValue="ABC"/>
    <reqQuantity>1</reqQuantity>
    </spareDescr>
    <spareDescr>
    <partRef manufacturerCodeValue="12345" partNumberValue="DEF"/>
    <reqQuantity>1</reqQuantity>
    </spareDescr>
    </spareDescrGroup>

Command and results:

    $ s1kd-repcheck -R <CIR> ... <DM>
    s1kd-repcheck: ERROR: <DM> (<line>): Part 12345/DEF not found.
