NAME
====

s1kd-dmref - Generate XML to reference a data module

SYNOPSIS
========

    s1kd-dmref [-tlih?] [<code>|<filename>]

DESCRIPTION
===========

The *s1kd-dmref* tool generates the XML for a &lt;dmRef&gt; element using the specified code or data module filename. When using a filename, it can parse the data module to include the issue, language, and/or title information in the reference.

OPTIONS
=======

-h -?  
Show the usage message.

-i  
Include the issue information in the reference (target must be a file)

-l  
Include the language information in the reference (target must be a file)

-t  
Include the dmTitle in the reference (target must be a file).

&lt;code&gt;|&lt;filename&gt;  
Either a data module code, including the prefix DMC or DME (for extended identification), or the filename of a data module.

EXAMPLE
=======

    $ s1kd-dmref DMC-S1000DTOOLS-A-00-08-00-00A-040A-D
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="S1000DTOOLS" systemDiffCode="A"
    systemCode="00" subSystemCode="0" subSubSystemCode="8" assyCode="00"
    disassyCode="00" disassyCodeVariant="A" infoCode="040"
    infoCodeVariant="A" itemLocationCode="D"/>
    </dmRefIdent>
    </dmRef>
