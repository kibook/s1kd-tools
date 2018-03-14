NAME
====

s1kd-ref - Generate XML to reference CSDB objects

SYNOPSIS
========

    s1kd-ref [-tlih?] [<code>|<filename>]

DESCRIPTION
===========

The *s1kd-ref* tool generates the XML for S1000D reference elements using the specified code or filename. When using a filename, it can parse the CSDB object to include the issue, language, and/or title information in the reference.

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
Either a code, including the prefix (DMC, PMC, etc.), or the filename of a CSDB object.

EXAMPLE
=======

    $ s1kd-ref DMC-S1000DTOOLS-A-00-08-00-00A-040A-D
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="S1000DTOOLS" systemDiffCode="A"
    systemCode="00" subSystemCode="0" subSubSystemCode="8" assyCode="00"
    disassyCode="00" disassyCodeVariant="A" infoCode="040"
    infoCodeVariant="A" itemLocationCode="D"/>
    </dmRefIdent>
    </dmRef>
