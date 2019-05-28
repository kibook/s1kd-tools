NAME
====

s1kd-appcheck - Validate applicability of S1000D CSDB objects

SYNOPSIS
========

    s1kd-appcheck [options] [<object>...]

DESCRIPTION
===========

The *s1kd-appcheck* tool validates the applicability of S1000D CSDB
objects, detecting potential errors that could occur when the object is
filtered. It can test objects either against the defined product
instances (using the PCT), or against all possible combinations of
product attribute and condition values relevant to an object (using the
ACT and CCT).

The s1kd-instance and s1kd-validate tools are used by default to perform
the actual validation.

OPTIONS
=======

-A, --act &lt;file&gt;  
Specify the ACT to read product attributes from, and to use to find the
CCT or PCT. This will override the ACT reference within the individual
objects being validated.

-a, --all  
Validate objects against all possible, relevant combinations of product
attribute and condition values as defined in the ACT and CCT. By
default, objects are validated only against the defined product
instances within the PCT.

-b, --brexcheck  
Validate objects with a BREX check (using the s1kd-brexcheck tool) in
addition to the schema check.

-C, --cct &lt;file&gt;  
Specify the CCT to read conditions from. This will override the CCT
reference within the ACT.

-d, --dir &lt;dir&gt;  
The directory to start searching for ACT/CCT/PCT data modules in. By
default, the current directory is used.

-e, --exec &lt;cmd&gt;  
The commands used to validate objects. Multiple commands can be used by
specifying this option multiple times. The objects will be passed to
each command on stdin, and the exit status of the command will be used
to determine if the object is valid (with a non-zero exit status
indicating it is invalid). This overrides the default commands
(s1kd-validate, and s1kd-brexcheck if -b is specified).

-f, --filenames  
Print the filenames of invalid objects.

-h, -?, --help  
Show help/usage message.

-k, --args &lt;args&gt;  
The arguments to the s1kd-instance tool when filtering objects prior to
validation.

-l, --list  
Treat input as a list of CSDB objects to validate.

-N, --omit-issue  
Assume that the issue/inwork numbers are omitted from object filenames
(they were created with the -N option).

-o, --output-valid  
Output valid CSDB objects to stdout.

-p, --pct &lt;file&gt;  
Specify the PCT to read product instances from. This will override the
PCT reference in the ACT.

-q, --quiet  
Quiet mode. Error messages will not be printed.

-r, --recursive  
Search for the ACT/CCT/PCT recursively.

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
Object(s) to validate.

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
The check completed successfully, and all CSDB objects were valid.

1  
The check completed successfully, but some CSDB objects were invalid.

2  
One or more CSDB objects could not be read.

EXAMPLE
=======

    $ s1kd-appcheck DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
