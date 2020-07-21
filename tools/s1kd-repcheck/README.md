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

-A, --all-refs  
Validate indirect tool/supply/part CIR references using the element
`<identNumber>`. Normally, only the direct reference elements
`<toolRef>`, `<supplyRef>` and `<partRef>` are validated.

-a, --all  
In addition to CIR data modules specified with -R or explicitly linked
in CIR references, allow CIR references to be resolved against any CIR
data modules that were specified as objects to check.

-D, --dump-xsl  
Dump the built-in XSLT used to extract CIR references.

-d, --dir &lt;dir&gt;  
The directory to start searching for CIR data modules in. By default,
the current directory is used.

-F, --valid-filenames  
Print the filenames of valid objects.

-f, --filenames  
Print the filenames of invalid objects.

-h, -?, --help  
Show help/usage message.

-L, --list-refs  
List CIR references found in objects instead of validating them.

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
be specified by using this option multiple times.

If "\*" is given for &lt;CIR&gt;, the tool will search for CIR data
modules automatically.

-r, --recursive  
Search for CIR data modules recursively.

-T, --summary  
Print a summary of the check after it completes, including statistics on
the number of objects that passed/failed the check.

-v, --verbose  
Verbose output. Specify multiple times to increase the verbosity.

-X, --xsl &lt;file&gt;  
Use custom XSLT to extract CIR references.

-x, --xml  
Print an XML report of the check.

-^, --remove-deleted  
Validate with elements that have a change type of "delete" removed. CIR
data modules with an issue type of "deleted" will also be ignored in the
automatic search when this option is specified.

--version  
Show version information.

&lt;object&gt;...  
Object(s) to check CIR references in.

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

Custom XSLT (-X)
----------------

What elements are extracted as CIR references for validating, and how
they are validated, can be configured through a custom XSLT script
specified with the -X (--xsl) option.

The custom XSLT script should add two attributes to elements which
should be validated as CIR references:

`name`  
A descriptive name for the CIR reference that can be used in reports.

For example, the CIR reference:

    <functionalItemRef functionalItemNumber="fin-00001"/>

could be named: "Functional item fin-00001".

`test`  
An XPath expression used to match the corresponding CIR identification
element.

For example, the test for the above example could be:
`//functionalItemIdent[@functionalItemNumber='fin-00001']`.

The namespace for both attributes must be:
`urn:s1kd-tools:s1kd-repcheck`

Example XSLT template to extract functional item references:

    <xsl:template match="functionalItemRef">
    <xsl:variable name="fin" select="@functionalItemNumber"/>
    <xsl:copy>
    <xsl:apply-templates select="@*"/>
    <xsl:attribute name="s1kd-repcheck:name">
    <xsl:text>Functional item </xsl:text>
    <xsl:value-of select="$fin"/>
    </xsl:attribute>
    <xsl:attribute name="s1kd-repcheck:test">
    <xsl:text>//functionalItemIdent[@functionalItemNumber='</xsl:text>
    <xsl:value-of select="$fin"/>
    <xsl:text>']</xsl:text>
    </xsl:attribute>
    <xsl:apply-templates select="node()"/>
    </xsl:copy>
    </xsl:template>

A custom script also allows validating non-standard types of "CIR"
references. For example, if a project wants to validate acronyms used in
data modules against a central repository of acronyms, this could be
done like so:

    <xsl:template match="acronym">
    <xsl:variable name="term" select="acronymTerm"/>
    <xsl:copy>
    <xsl:apply-templates select="@*"/>
    <xsl:attribute name="s1kd-repcheck:name">
    <xsl:text>Acronym </xsl:text>
    <xsl:value-of select="$term"/>
    </xsl:attribute>
    <xsl:attribute name="s1kd-repcheck:test">
    <xsl:text>//acronym[acronymTerm = '</xsl:text>
    <xsl:value-of select="$term"/>
    <xsl:text>']</xsl:text>
    </xsl:attribute>
    <xsl:apply-templates select="node()"/>
    </xsl:copy>
    </xsl:template>

As there is no standard "acronym" CIR type, the object containing the
repository would need to be specified explicitly with -R.

The built-in XSLT for extracting CIR references can be dumped as a
starting point for a custom script by specifying the -D (--dump-xsl)
option.

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
