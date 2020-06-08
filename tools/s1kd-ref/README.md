NAME
====

s1kd-ref - Generate XML to reference CSDB objects

SYNOPSIS
========

    s1kd-ref [-cdfgiLlqRrStuvh?] [-$ <issue>] [-s <src>] [-T <opts>]
             [-x <xpath>] [-3 <file>] [-o <dst>] [<code>|<file> ...]

DESCRIPTION
===========

The *s1kd-ref* tool generates the XML for S1000D reference elements
using the specified code or filename. When using a filename, it can
parse the CSDB object to include the issue, language, and/or title
information in the reference.

OPTIONS
=======

-$, --issue &lt;issue&gt;  
Output XML for the specified issue of S1000D.

-c, --content  
When using the -T option, only transform textual references found in the
content section of CSDB objects.

-d, --include-date  
Include the issue date in the reference (target must be a file)

-f, --overwrite  
Overwrite source data module instead of writing to stdout.

-g, --guess-prefix  
Accept references which do not include a standard prefix (e.g., "DMC-",
"PMC-") and guess what they are based on their format and, when using
the -T option, the XML context in which they occur.

-h, -?, --help  
Show the usage message.

-i, --include-issue  
Include the issue information in the reference (target must be a file)

-L, --list  
Treat input as a list of CSDB objects.

-l, --include-lang  
Include the language information in the reference (target must be a
file)

-o, --out &lt;dst&gt;  
Output to &lt;dst&gt; instead of stdout.

-q, --quiet  
Quiet mode. Do not print errors.

-R, --repository-id  
Generate a `<repositorySourceDmIdent>` for a data module.

-r, --add  
Add the generated reference to the source data module's `refs` table and
output the modified data module to stdout.

-S, --source-id  
Generate a `<sourceDmIdent>` (for data modules) or `<sourcePmIdent>`
(for publication modules).

-s, --source &lt;src&gt;  
Specify a source data module &lt;src&gt; to add references to when using
the -r option.

-T, --transform &lt;opts&gt;  
Transform textual references into the appropriate XML within text nodes
in the XML document(s) specified. The textual references must include
the standard prefixes (e.g., "DMC-", "PMC-'), unless the -p option is
specified. &lt;opts&gt; is a sequence of characters from "CDEGLPSY", for
comment, data module, external publication, ICN, DML, publication
module, SCORM content package and CSN references respectively. If "all"
is given, then all types of references will be transformed.

-t, --include-title  
Include the title in the reference (target must be a file).

-u, --include-url  
Include the full URL/filename of the reference with the `xlink:href`
attribute.

-v, --verbose  
Verbose output.

-x, --xpath &lt;xpath&gt;  
When using the -T option, this specifies which nodes to transform
textual references in. By default, only the elements which can contain
each type of reference are considered.

-3, --externalpubs &lt;file&gt;  
Use a custom `.externalpubs` file.

--version  
Show version information.

&lt;code&gt;\|&lt;file&gt;  
Either a code, including the prefix (DMC, PMC, etc.), or the filename of
a CSDB object.

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

`.externalpubs` file
--------------------

The `.externalpubs` file contains definitions of external publication
references. This can be used to generate the XML for an external
publication reference by specifying the external publication code.

Example of a `.externalpubs` file:

    <externalPubs>
    <externalPubRef>
    <externalPubRefIdent>
    <externalPubCode>ABC</externalPubCode>
    <externalPubTitle>ABC Manual</externalPubTitle>
    </externalPubRefIdent>
    </externalPubRef>
    </externalPubs>

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

Reference to a SCORM content package:

    $ s1kd-ref SMC-EX-12345-00001-00
    <scormContentPackageRef>
    <scormContentPackageRefIdent>
    <scormContentPackageCode
    modelIdentCode="EX"
    scormContentPackageIssuer="12345"
    scormContentPackageNumber="00001"
    scormContentPackageVolume="00"/>
    </scormContentPackageRefIdent>
    </scormContentPackageRef>

Source identification for a data module:

    $ s1kd-ref -S DMC-EX-A-00-00-00-00A-040A-D_001-00_EN-CA.XML
    <sourceDmIdent>
    <dmCode modelIdentCode="EX" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="040" infoCodeVariant="A"
    itemLocationCode="D"/>
    <language languageIsoCode="en" countryIsoCode="CA"/>
    <issueInfo issueNumber="001" inWork="00"/>
    </sourceDmIdent>

Source identification for a publication module:

    $ s1kd-ref -S PMC-EX-12345-00001-00_001-00_EN-CA.XML
    <sourcePmIdent>
    <pmCode modelIdentCode="EX" pmIssuer="12345" pmNumber="00001"
    pmVolume="00"/>
    <language languageIsoCode="en" countryIsoCode="CA"/>
    <issueInfo issueNumber="001" inWork="00"/>
    </sourcePmIdent>

Source identification for a SCORM content package:

    $ s1kd-ref -S SMC-EX-12345-00001-00_001-00_EN-CA.XML
    <sourceScormContentPackageIdent>
    <scormContentPackageCode
    modelIdentCode="EX"
    scormContentPackageIssuer="12345"
    scormContentPackageNumber="00001"
    scormContentPackageVolume="00"/>
    <language languageIsoCode="en" countryIsoCode="CA"/>
    <issueInfo issueNumber="000" inWork="01"/>
    </sourceScormContentPackageIdent>

Repository source identification for a CIR data module:

    $ s1kd-ref -R DMC-EX-A-00-00-00-00A-00GA-D_001-00_EN-CA.XML
    <repositorySourceDmIdent>
    <dmCode modelIdentCode="EX" systemDiffCode="A" systemCode="00"
    subSystemCode="0" subSubSystemCode="0" assyCode="00" disassyCode="00"
    disassyCodeVariant="A" infoCode="00G" infoCodeVariant="A"
    itemLocationCode="D"/>
    <language languageIsoCode="en" countryIsoCode="CA"/>
    <issueInfo issueNumber="001" inWork="00"/>
    </repositorySourceDmIdent>

Reference to an external publication:

    $ s1kd-ref ABC
    <externalPubRef>
    <externalPubRefIdent>
    <externalPubCode>ABC</externalPubCode>
    </externalPubRefIdent>
    </externalPubRef>

Reference to an external publication (from the `.externalpubs` file):

    $ s1kd-ref ABC
    <externalPubRef>
    <externalPubRefIdent>
    <externalPubCode>ABC</externalPubCode>
    <externalPubTitle>ABC Manual</externalPubTitle>
    </externalPubRefIdent>
    </externalPubRef>
