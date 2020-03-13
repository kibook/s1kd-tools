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

By default, the tool validates an object against only the product
attribute and condition values which are explicitly used within the
object. The products check (-t) and full check (-a) modes allow objects
to be checked for issues with implicit applicability, that is, product
attribute or condition values which are not explicitly used within an
object, but may still affect it.

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

-c, --custom  
Perform a customized check. The default standalone applicability check
is disabled. This can then be combined with the -s option, to only check
that all product attributes and conditions are defined in the ACT and
CCT respectively, and/or the -n option, to only check nested
applicability annotations. If neither of these options are specified, no
checks will be performed.

-D, --remove-deleted  
Validate objects with elements that have a change type of "delete"
removed.

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

-K, --filter &lt;cmd&gt;  
The command used to filter objects prior to validation. The objects will
be passed to the command on stdin, and the filters will be supplied as
arguments in the form of "`-s <ident>:<type>=<value>`". This overrides
the default command (s1kd-instance).

-k, --args &lt;args&gt;  
The arguments to the filter command when filtering objects prior to
validation.

-l, --list  
Treat input as a list of CSDB objects to validate.

-N, --omit-issue  
Assume that the issue/inwork numbers are omitted from object filenames
(they were created with the -N option).

-n, --nested  
Check that all product attribute and condition values used in nested
applicability annotations are subsets of the values used in their
parents.

-o, --output-valid  
Output valid CSDB objects to stdout.

-P, --pct &lt;file&gt;  
Specify the PCT to read product instances from. This will override the
PCT reference in the ACT.

-p, --progress  
Display a progress bar.

-q, --quiet  
Quiet mode. Error messages will not be printed.

-r, --recursive  
Search for the ACT/CCT/PCT recursively.

-s, --strict  
Check whether product attributes and conditions used by an object are
declared in the ACT and CCT respectively.

-T, --summary  
Print a summary of the check after it completes, including statistics on
the number of objects that passed/failed the check.

-t, --products  
Validate objects against the defined product instances within the PCT.

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

EXIT STATUS
===========

0  
The check completed successfully, and all CSDB objects were valid.

1  
The check completed successfully, but some CSDB objects were invalid.

2  
One or more CSDB objects could not be read.

3  
The number of CSDB objects specified exceeded the available memory.

EXAMPLES
========

Standalone validation
---------------------

Consider the following data module snippet:

    <dmodule>
    ...
    <applic>
    <displayText>
    <simplePara>Version: A or Version: B</simplePara>
    </displayText>
    <evaluate andOr="or">
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="B"/>
    </evaluate>
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

There are two versions of the product, A and B, and the data module is
meant to apply to both.

By itself, the data module is valid:

    $ s1kd-validate -v <DM>
    s1kd-validate: SUCCESS: <DM> validates against schema <url>

Checking it with this tool, however, reveals an issue:

    $ s1kd-appcheck <DM>
    s1kd-appcheck: ERROR: <DM> is invalid when:
    s1kd-appcheck: ERROR:   prodattr version = A

When the data module is filtered for version A, the first levelled
paragraph will be removed, which causes the reference to it in the
second levelled paragraph to become broken.

Full validation
---------------

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
    <applic id="app-IcyOrHot">
    <evaluate andOr="or">
    <assert
    applicPropertyIdent="weather"
    applicPropertyType="condition"
    applicPropertyValues="Icy"/>
    <assert
    applicPropertyIdent="weather"
    applicPropertyType="condition"
    applicPropertyValues="Hot"/>
    </applic>
    </referencedApplicGroup>
    ...
    <proceduralStep>
    <para>Locate the handle.</para>
    </proceduralStep>
    <proceduralStep id="stp-0001" applicRefId="app-IcyOrHot">
    <para>Put on gloves prior to touching the handle.</para>
    </proceduralStep>
    <proceduralStep>
    <para>Grab the handle and turn it clockwise.</para>
    </proceduralStep>
    ...
    <proceduralStep>
    <para>Remove the gloves you put on in <internalRef internalRefId="stp-0001"/>.</para>
    </proceduralStep>
    ...
    </dmodule>

Once again, this data module is valid by itself:

    $ s1kd-validate -v <DM>
    s1kd-validate: SUCCESS: <DM> validates against schema <url>

This time, however, it also initially appears valid when this tool is
used:

    $ s1kd-appcheck -v <DM>
    s1kd-appcheck: SUCCESS: <DM> passed the applicability check.

However, now consider this snippet from the CCT:

    <condCrossRefTable>
    ...
    <condType id="weatherType">
    <name>Weather type</name>
    <descr>Possible types of weather conditions.</descr>
    <enumeration applicPropertyValues="Normal"/>
    <enumeration applicPropertyValues="Icy"/>
    <enumeration applicPropertyValues="Hot"/>
    </condType>
    ...
    <cond id="weather" condTypeRefId="weatherType">
    <name>Weather</name>
    <descr>The current weather conditions.</descr>
    </cond>
    ...
    </condCrossRefTable>

There is a third value for the `weather` condition which is not
explicitly used within the data module, and therefore will not be
validated against in the default standalone check. When `weather` has a
value of `Normal`, the cross-reference in the last step in the example
above becomes broken.

To catch errors with implicit applicability, the full check (-a) can be
used instead, which reads the values to check not from the data module
itself, but from the ACT and CCT referenced by the data module:

    $ s1kd-appcheck -a <DM>
    s1kd-appcheck: ERROR: <DM> is invalid when:
    s1kd-appcheck: ERROR:   condition weather = Normal

This can also be fixed by making the applicability of the data module
explicit:

    <applic>
    <displayText>
    <simplePara>Weather: Normal or Weather: Icy or
    Weather: Hot</simplePara>
    </displayText>
    <evaluate andOr="or">
    <assert
    applicPropertyIdent="weather"
    applicPropertyType="condition"
    applicPropertyValues="Normal"/>
    <assert
    applicPropertyIdent="weather"
    applicPropertyType="condition"
    applicPropertyValues="Icy"/>
    <assert
    applicPropertyIdent="weather"
    applicPropertyType="condition"
    applicPropertyValues="Hot"/>
    </evaluate>
    </applic>

In which case, the standalone check will now also detect the error:

    $ s1kd-appcheck <DM>
    s1kd-appcheck: ERROR: <DM> is invalid when:
    s1kd-appcheck: ERROR:   condition weather = Normal

Nested applicability annotations
--------------------------------

Consider the following data module snippet:

    <dmodule>
    ...
    <applic>
    <displayText>
    <simplePara>Version: A, B</simplePara>
    </displayText>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="B"/>
    </applic>
    ...
    <referencedApplicGroup>
    <applic id="app-C">
    <displayText>
    <simplePara>Version: C</simplePara>
    </displayText>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="C"/>
    </applic>
    </referencedApplicGroup>
    ...
    <proceduralStep>
    <para>Step A</para>
    </proceduralStep>
    <proceduralStep applicRefId="app-C">
    <para>Step B</para>
    </proceduralStep>
    <proceduralStep>
    <para>Step C</para>
    </proceduralStep>
    ...
    </dmodule>

Here, the whole data module is applicable to versions A and B, but an
individual step has been made applicable to version C. Normally, this is
not reported as an error, since the removal of this step would not cause
the data module to become invalid:

    $ s1kd-appcheck -v <DM>
    s1kd-appcheck: SUCCESS: <DM> passed the applicability check

However, the content is essentially useless, since it will never appear.
The -n option will report when the applicability of an element is
incompatible with the applicability of any parent elements or the whole
object:

    $ s1kd-appcheck -n <DM>
    s1kd-appcheck: ERROR: <DM>: proceduralStep on line 62 is applicable
    when prodattr version = C, which is not a subset of the applicability
    of the whole object.
