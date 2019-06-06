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
filtered.

There are three methods for validating applicability:

Products check (-p)  
Check that objects are valid for all product instances, as defined in
the PCT. Conditions that are not associated with a product instance will
not be checked.

Standalone check (-s)  
Check that objects are valid for all possible combinations of product
attribute and condition values that are used within the object. If
applicability is always given explicitly within objects, this is the
most efficient method.

Full check (-a)  
Check that objects are valid for all possible combinations of product
attribute and condition values, as defined in the ACT and CCT. This is
the most complete check.

If no method is selected, the tool will by default check that all
product attributes and conditions used by an object are defined in the
ACT and CCT respectively. This can be combined with any of the above
checks by specifying the -c option.

The s1kd-instance and s1kd-validate tools are used by default to perform
the actual validation.

OPTIONS
=======

-A, --act &lt;file&gt;  
Specify the ACT to read product attributes from, and to use to find the
CCT or PCT. This will override the ACT reference within the individual
objects being validated.

-a, --all  
Validate objects against all possible combinations of relevant product
attribute and condition values as defined in the ACT and CCT. Relevant
product attributes and conditions are those that are used by an object
with any value.

-b, --brexcheck  
Validate objects with a BREX check (using the s1kd-brexcheck tool) in
addition to the schema check.

-C, --cct &lt;file&gt;  
Specify the CCT to read conditions from. This will override the CCT
reference within the ACT.

-c, --strict  
Check whether product attributes and conditions used by an object are
declared in the ACT and CCT respectively.

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

-P, --pct &lt;file&gt;  
Specify the PCT to read product instances from. This will override the
PCT reference in the ACT.

-p, --products  
Validate objects against the defined product instances within the PCT.

-q, --quiet  
Quiet mode. Error messages will not be printed.

-r, --recursive  
Search for the ACT/CCT/PCT recursively.

-s, --standalone  
Perform a standalone applicability check without an ACT, CCT, or PCT,
using only the applicability property values contained in each object.

-T, --summary  
Print a summary of the check after it completes, including statistics on
the number of objects that passed/failed the check.

-v, --verbose  
Verbose output. Specify multiple times to increase the verbosity.

-x, --xml  
Print an XML report of the check.

-\~, --dependencies  
Check with CCT dependency tests added to assertions which use the
dependant values.

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

Consider the following data module snippet:

    <dmodule>
    ...
    <applic>
    <displayText>
    <simplePara>All</simplePara>
    </displayText>
    </applic>
    ...
    <referencedApplicGroup>
    <applic id="app-VersionB">
    <assert applicPropertyIdent="version" applicPropertyType="prodattr"
    applicPropertyValues="B"/>
    </applic>
    </referencedApplicGroup>
    ...
    <levelledPara id="par-0001" applicRefId="app-VersionB">
    <title>Features of version B</title>
    <para>...</para>
    </levelledPara>
    ...
    <levelledPara>
    <title>More information</title>
    <para>...</para>
    <para>Refer to <internalRef internalRefId="par-0001"/>.</para>
    </levelledPara>
    ...
    </dmodule>

And consider this snippet of the PCT associated with the above data
module:

    <productCrossRefTable>
    <product id="Version_A">
    <assign applicPropertyIdent="version" applicPropertyType="prodattr"
    applicPropertyValue="A"/>
    </product>
    <product id="Version_B">
    <assign applicPropertyIdent="version" applicPropertyType="prodattr"
    applicPropertyValue="B"/>
    </product>
    </productCrossRefTable>

There are two versions of the product, A and B, and the data module is
meant to apply to both.

By itself, the data module is valid:

    $ s1kd-validate -v <DM>
    s1kd-validate: SUCCESS: <DM> validates against schema <url>

Checking it with this tool, however, reveals an issue:

    $ s1kd-appcheck -p <DM>
    s1kd-appcheck: ERROR: <DM> is invalid for product Version_A

When the data module is filtered for version A, the first levelled
paragraph will be removed, which causes the reference to it in the
second levelled paragraph to become broken.
