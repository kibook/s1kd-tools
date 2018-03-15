NAME
====

s1kd-ref - Generate XML to reference CSDB objects

SYNOPSIS
========

    s1kd-ref [-filrth?] [-s <src>] [-o <dst>]
             [<code>|<filename>]...

DESCRIPTION
===========

The *s1kd-ref* tool generates the XML for S1000D reference elements using the specified code or filename. When using a filename, it can parse the CSDB object to include the issue, language, and/or title information in the reference.

OPTIONS
=======

-f  
Overwrite source data module instead of writing to stdout.

-h -?  
Show the usage message.

-i  
Include the issue information in the reference (target must be a file)

-l  
Include the language information in the reference (target must be a file)

-o &lt;dst&gt;  
Output to &lt;dst&gt; instead of stdout.

-r  
Add the generated reference to the source data module's `refs` table rather than printing the XML to stdout.

-s &lt;src&gt;  
Specify a source data module &lt;src&gt; to add references to when using the -r option.

-t  
Include the title in the reference (target must be a file).

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
