NAME
====

s1kd-neutralize - S1000D IETP neutral translation of CSDB objects

SYNOPSIS
========

    s1kd-neutralize [-o <file>] [-flnvh?] [<object>...]

DESCRIPTION
===========

Generates neutral metadata for the specified CSDB objects. This
includes:

-   XLink attributes for references, using the S1000D URN scheme.

-   RDF and Dublin Core metadata.

OPTIONS
=======

-f, --overwrite  
Overwrite specified CSDB object(s) automatically.

-h, -?, --help  
Show usage message.

-l, --list  
Treat input (stdin or arguments) as lists of CSDB objects to neutralize,
rather than CSDB objects themselves.

-n, --namespace  
Include the IETP namespaces for data module and publication module
elements.

-o, --out &lt;file&gt;  
Output neutralized CSDB object XML to &lt;file&gt; instead of stdout.

-v, --verbose  
Verbose output.

--version  
Show version information.

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

EXAMPLE
=======

    $ DMOD=DMC-XLINKTEST-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    $ xmllint --xpath "//description/dmRef" $DMOD
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="XLINKTEST" systemDiffCode="A"
    systemCode="00" subSystemCode="0" subSubSystemCode="0" assyCode="01"
    disassyCode="00" disassyCodeVariant="A" infoCode="040"
    infoCodeVariant="A" itemLocationCode="D"/>
    </dmRefIdent>
    <dmRefAddressItems>
    <dmTitle>
    <techName>XLink test</techName>
    <infoName>Referenced data module</infoName>
    </dmTitle>
    </dmRefAddressItems>
    </dmRef>

    $ s1kd-neutralize $DMOD | xmllint --xpath "//description/dmRef" -
    <dmRef xlink:type="simple"
    xlink:href="URN:S1000D:DMC-XLINKTEST-A-00-00-01-00A-040A-D"
    xlink:title="XLink test - Referenced data module">
    [...]
    </dmRef>
