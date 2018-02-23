NAME
====

s1kd-neutralize - S1000D IETP neutral translation of data modules

SYNOPSIS
========

    s1kd-neutralize [-o <file>] [-fh?] [<data module> ...]

DESCRIPTION
===========

Generates neutral metadata for the specified data modules. This includes:

-   XLink attributes for references, using the S1000D URN scheme.

-   RDF and Dublin Core metadata.

OPTIONS
=======

-f  
Overwrite specified data module(s) automatically.

-h -?  
Show usage message.

-o &lt;file&gt;  
Output neutralized data module XML to &lt;file&gt; instead of stdout.

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
