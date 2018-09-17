NAME
====

s1kd-ref - Generate XML to reference CSDB objects

SYNOPSIS
========

    s1kd-ref [-filrtdh?] [-$ <issue>] [-s <src>] [-o <dst>]
             [<code>|<filename>]...

DESCRIPTION
===========

The *s1kd-ref* tool generates the XML for S1000D reference elements using the specified code or filename. When using a filename, it can parse the CSDB object to include the issue, language, and/or title information in the reference.

OPTIONS
=======

-$ &lt;issue&gt;  
Output XML for the specified issue of S1000D.

-d  
Include the issue date in the reference (target must be a file)

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
Add the generated reference to the source data module's `refs` table and output the modified data module to stdout.

-s &lt;src&gt;  
Specify a source data module &lt;src&gt; to add references to when using the -r option.

-t  
Include the title in the reference (target must be a file).

--version  
Show version information.

&lt;code&gt;|&lt;filename&gt;  
Either a code, including the prefix (DMC, PMC, etc.), or the filename of a CSDB object.

EXAMPLES
========

Reference to data module with data module code:

    $ s1kd-ref DMC-EX-A-00-00-00-00A-040A-D
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="EX" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="040" infoCodeVariant="A"
    itemLocationCode="D"/>
    </dmRefIdent>
    </dmRef>

Reference to data module with data module code and issue/language:

    $ s1kd-ref -il DMC-EX-A-00-00-00-00A-040A-D_001-03_EN-CA
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="EX" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="040" infoCodeVariant="A"
    itemLocationCode="D"/>
    <issueInfo issueNumber="001" inWork="03"/>
    <language languageIsoCode="en" countryIsoCode="CA"/>
    </dmRefIdent>
    </dmRef>

Reference to data module with all information, from a file:

    $ s1kd-ref -dilt DMC-EX-A-00-00-00-00A-040A-D_001-03_EN-CA.XML
    <dmRef>
    <dmRefIdent>
    <dmCode modelIdentCode="EX" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="040" infoCodeVariant="A"
    itemLocationCode="D"/>
    <issueInfo issueNumber="001" inWork="03"/>
    <language languageIsoCode="en" countryIsoCode="CA"/>
    </dmRefIdent>
    <dmRefAddressItems>
    <dmTitle>
    <techName>Example</techName>
    <infoName>Description</infoName>
    </dmTitle>
    <issueDate year="2018" month="06" day="25"/>
    </dmRefAddressItems>
    </dmRef>

Reference to a catalog sequence number:

    $ s1kd-ref CSN-EX-A-00-00-00-01A-004A-D
    <catalogSeqNumberRef modelIdentCode="EX" systemDiffCode="A"
    systemCode="00" subSystemCode="0" subSubSystemCode="0" assyCode="00"
    figureNumber="01" figureNumberVariant="A" item="004" itemVariant="A"
    itemLocationCode="D"/>

Reference to a comment:

    $ s1kd-ref COM-EX-12345-2018-00001-Q
    <commentRef>
    <commentRefIdent>
    <commentCode modelIdentCode="EX" senderIdent="12345"
    yearOfDataIssue="2018" seqNumber="00001" commentType="q"/>
    </commentRefIdent>
    </commentRef>

Reference to a data management list:

    $ s1kd-ref DML-EX-12345-C-2018-00001
    <dmlRef>
    <dmlRefIdent>
    <dmlCode modelIdentCode="EX" senderIdent="12345" dmlType="c"
    yearOfDataIssue="2018" seqNumber="00001"/>
    </dmlRefIdent>
    </dmlRef>

Reference to an information control number:

    $ s1kd-ref ICN-EX-A-000000-A-00001-A-001-01
    <infoEntityRef infoEntityRefIdent="ICN-EX-A-000000-A-00001-A-001-01"/>

Reference to a publication module:

    $ s1kd-ref PMC-EX-12345-00001-00
    <pmRef>
    <pmRefIdent>
    <pmCode modelIdentCode="EX" pmIssuer="12345" pmNumber="00001"
    pmVolume="00"/>
    </pmRefIdent>
    </pmRef>
