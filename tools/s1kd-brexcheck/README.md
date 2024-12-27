# NAME

s1kd-brexcheck - Validate S1000D CSDB objects against BREX data modules

# SYNOPSIS

    s1kd-brexcheck [-b <brex>] [-d <dir>] [-I <path>] [-w <severities>]
                   [-X <version>] [-F|-f] [-BceLlNnopqrS[tu]sTvx8^h?]
                   [<object>...]

# DESCRIPTION

The *s1kd-brexcheck* tool validates S1000D CSDB objects using the
context, SNS, and/or notation rules of one or multiple Business Rules
EXchange (BREX) data modules. All errors are displayed with the
\<objectUse\> message, the line number, and a representation of the
invalid XML tree.

# OPTIONS

  - \-B, --default-brex  
    Check each input object against the appropriate built-in S1000D
    default BREX only. The actual BREX reference of each object is
    ignored.

  - \-b, --brex \<brex\>  
    Check the CSDB objects against this BREX. Multiple BREX data modules
    can be specified by adding this option multiple times. When no BREX
    data modules are specified, the BREX data module referenced in
    \<brexDmRef\> in the CSDB object is attempted to be used instead.

  - \-c, --values  
    When a context rule defines values for an object (objectValue),
    check if the value of each object is within the allowed set of
    values.

  - \-d, --dir \<dir\>  
    Directory to start searching for BREX data modules in. By default,
    the current directory is used.

  - \-e, --ignore-empty  
    Ignore check for empty or non-XML documents.

  - \-F, --valid-filenames  
    Print the filenames of CSDB objects with no BREX/SNS errors.

  - \-f, --filenames  
    Print the filenames of CSDB objects with BREX/SNS errors.

  - \-h, -?, --help  
    Show the help/usage message.

  - \-I, --include \<path\>  
    Add a search path for BREX data modules. By default, only the
    current directory is searched.

  - \-L, --list  
    Treat input as a list of object filenames to check, rather than an
    object itself.

  - \-l, --layered  
    Use the layered BREX concept. BREX data modules referenced by other
    BREX data modules (either specified with -b or referenced by the
    specified CSDB objects) will also be checked against.

  - \-N, --omit-issue  
    Assume that the issue/inwork numbers are omitted from object
    filenames (they were created with the -N option).

  - \-n, --notations  
    Check notation rules. Any notation names listed in any of the BREX
    data modules with attribute `allowedNotationFlag` set to "1" or
    omitted are considered valid notations. If a notation in a CSDB
    object is not present or has `allowedNotationFlag` set to "0", an
    error will be returned.
    
    For notations not included but not explicitly excluded, the
    `objectUse` of the first inclusion rule will be returned with the
    error. For explicitly excluded notations, the `objectUse` of the
    explicit exclusion rule is returned.

  - \-o, --output-valid  
    Output valid CSDB objects to stdout.

  - \-p, --progress  
    Display a progress bar.

  - \-q, --quiet  
    Quiet mode. No errors are printed, they are only indicated via the
    exit status.

  - \-r, --recursive  
    Search for BREX data modules recursively.

  - \-S\[tu\], --sns \[--strict|--unstrict\]  
    Check Standard Numbering System (SNS) rules. The SNS of each
    specified data module is checked against the combination of all SNS
    rules of all specified BREX data modules.

  - \-s, --short  
    Use shortened, single-line messages to report BREX errors instead of
    multiline indented messages.

  - \-T, --summary  
    Print a summary of the check after it completes, including
    statistics on the number of documents that passed/failed the check.

  - \-v, --verbose  
    Verbose mode. The success or failure of each test is printed
    explicitly.

  - \-w, --severity-levels \<file\>  
    Specify a list of severity levels for business rules.

  - \-X, --xpath-version \<version\>  
    Force the specified version of XPath to be used when evaluating the
    object paths of BREX rules.

  - \-x, --xml  
    Output an XML report.

  - \-8, --deep-copy-nodes  
    Include a deep copy of invalid nodes on the XML report (-x). By
    default, only a shallow copy of the node is included (the node and
    its attributes but no children).

  - \-^, --remove-deleted  
    Check the CSDB objects with elements that have a change type of
    "delete" removed.

  - \--version  
    Show version information.

  - \--zenity-progress  
    Print progress information in the zenity --progress format.

In addition, the following options allow configuration of the XML
parser:

  - \--dtdload  
    Load the external DTD.

  - \--huge  
    Remove any internal arbitrary parser limits.

  - \--net  
    Allow network access to load external DTD and entities.

  - \--noent  
    Resolve entities.

  - \--parser-errors  
    Emit errors from parser.

  - \--parser-warnings  
    Emit warnings from parser.

  - \--xinclude  
    Do XInclude processing.

  - \--xml-catalog \<file\>  
    Use an XML catalog when resolving entities. Multiple catalogs may be
    loaded by specifying this option multiple times.

## Business rule severity levels (`.brseveritylevels`)

The attribute `brSeverityLevel` on a BREX rule allows for distinguishing
different kinds of errors. The `.brseveritylevels` file contains a list
of severity levels, their user-defined type, and optionally if they
should not be counted as true errors (causing the tool to return a
"failure" status) but merely warnings.

By default, the program will search the current directory and parent
directories for a file named `.brseveritylevels`, but any file can be
specified by using the -w option.

An example of the format of this file is given below:

    <?xml version="1.0"?>
    <brSeverityLevels>
    <brSeverityLevel value="brsl01" fail="yes">Error</brSeverityLevel>
    <brSeverityLevel value="brsl02" fail="no">Warning</brSeverityLevel>
    </brSeverityLevels>

When the attribute `fail` has a value of `"yes"` (or is not included),
BREX errors pertaining to rules with the given severity level value will
be counted as errors. When it is `"no"`, the errors are still displayed
but are not counted as errors in the exit status code of the tool.

## Normal, strict and unstrict SNS check (-S, -St, -Su)

There are three modes for SNS checking: normal, strict, and unstrict.
The main difference between them is how they handle the optional levels
of an SNS description in the BREX.

\-St enables *strict* SNS checking. By default, the normal SNS check
(-S) will assume optional elements snsSubSystem, snsSubSubSystem, and
snsAssy exist with an snsCode of "0" ("00" or "0000" for snsAssy) when
their parent element does not contain any of each. This provides a
shorthand, such that

    <snsSystem>
    <snsCode>00</snsCode>
    <snsTitle>General</snsTitle>
    </snsSystem>

is equivalent to

    <snsSystem>
    <snsCode>00</snsCode>
    <snsTitle>General</snsTitle>
    <snsSubSystem>
    <snsCode>0</snsCode>
    <snsTitle>General</snsTitle>
    <snsSubSubSystem>
    <snsCode>0</snsCode>
    <snsTitle>General</snsTitle>
    <snsAssy>
    <snsCode>00</snsCode>
    <snsTitle>General</snsTitle>
    </snsAssy>
    </snsSubSubSystem>
    </snsSubSystem>
    </snsSystem>

Using strict checking will disable this shorthand, and missing optional
elements will result in an error.

\-Su enables *unstrict* SNS checking. The normal SNS check (-S)
shorthand mentioned above only allows SNS codes of "0" to be omitted
from the SNS rules. Using unstrict checking, *any* code used will not
produce an error when the relevant optional elements are omitted. This
means that given the following...

    <snsSystem>
    <snsCode>00</snsCode>
    <snsTitle>General</snsTitle>
    </snsSystem>

...SNS codes of 00-00-0000 through 00-ZZ-ZZZZ are considered valid.

## Object value checking (-c)

There are two ways to restrict the allowable values of an object in a
BREX rule. One is to use the XPath expression itself. For example, this
expression will match any `securityClassification` attribute whose value
is neither `"01"` nor `"02"`, and because the `allowedObjectFlag` is
`"0"`, will generate a BREX error if any match is found:

    <objectPath allowedObjectFlag="0">
    //@securityClassification[
    . != '01' and
    . != '02'
    ]
    </objectPath>

However, this method can lead to fairly complex expressions and requires
a reversal of logic. The BREX schema provides an alternative method
using the element `objectValue`:

    <structureObjectRule>
    <objectPath allowedObjectFlag="2">
    //@securityClassification
    </objectPath>
    <objectValue valueAllowed="01">Unclassified</objectValue>
    <objectValue valueAllowed="02">Classified</objectValue>
    </structureObjectRule>

Specifying the -c option will enable checking of these types of rules,
and if the value is not within the allowed set a BREX error will be
reported. The `valueForm` attribute can be used to specify what kind of
notation the `valueAllowed` attribute will contain:

  - `"single"` - A single, exact value.

  - `"range"` - Values given in the S1000D range/set notation, e.g.
    `"a~c"` or `"a|b|c"`.

  - `"pattern"` - A regular expression.

The s1kd-brexcheck tool supports all three types. If the `valueForm`
attribute is omitted, it will assume the value is in the `"single"`
notation.

## XPath support

By default, s1kd-brexcheck supports only XPath 1.0, with partial support
for EXSLT functions.

If experimental XPath 2.0 support is enabled at compile-time,
s1kd-brexcheck will automatically choose a version of XPath based on the
S1000D issue of the BREX data module:

  - 3.0 and lower  
    XPath 1.0

  - 4.0 and up  
    XPath 2.0

The -X (--xpath-version) option can be specified to force a particular
version of XPath to be used regardless of issue. Information on which
XPath versions are supported can be obtained from the --version option.

If the XPath given for the `<objectPath>` of a rule is invalid, the rule
will be ignored when validating objects. A warning will be printed to
stderr, and the XML report will contain an `<xpathError>` element for
each error.

# EXIT STATUS

  - 0  
    Check completed successfully, and no CSDB objects had BREX errors.

  - 1  
    Check completed successfully, but some CSDB objects had BREX errors.

  - 2  
    One or more CSDB objects specified could not be read.

  - 3  
    A referenced BREX data module could not be found.

  - 4  
    The XPath version specified is unsupported.

  - 5  
    The number of paths or CSDB objects specified exceeded the available
    memory.

# EXAMPLE

    $ DMOD=DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    $ BREX=DMC-S1000D-G-04-10-0301-00A-022A-D_001-00_EN-US.XML
    $ cat $DMOD
    [...]
    <listItem id="stp-0001">
    <para>List items shouldn't be used as steps...</para>
    </listItem>
    [...]
    <para>Refer to <internalRef internalRefId="stp-0001"
    internalRefTargetType="irtt08"/>.</para>
    [...]
    
    $ s1kd-brexcheck -b $BREX $DMOD
    BREX ERROR: DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
      BREX: DMC-S1000D-G-04-10-0301-00A-022A-D_001-00_EN-US.XML
      BREX-S1-00052
      Only when the reference target is a step can the value of attribute
    internalRefTargetType be irtt08 (Chap 3.9.5.2.1.2, Para 2.1).
      line 52 (/dmodule[1]/content[1]/description[1]/para[2]/
    internalRef[1]):
        ELEMENT internalRef
          ATTRIBUTE internalRefTargetType
            TEXT
              content=irtt08
          ATTRIBUTE internalRefId
            TEXT
              content=stp-0001

Example of XML report format for the above:

    <?xml version="1.0"?>
    <brexCheck>
    <document path="DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML">
    <brex path="DMC-S1000D-G-04-10-0301-00A-022A-D_001-00_EN-US.XML">
    <error fail="yes">
    <brDecisionRef brDecisionIdentNumber="BREX-S1-00052"/>
    <objectPath allowedObjectFlag="0">...</objectPath>
    <objectUse>Only when the refernce target is a step can the value of
    attribute internalRefTargetType be irtt08
    (Chap 3.9.5.2.1.2, Para 2.1).</objectUse>
    <object line="52"
    xpath="/dmodule[1]/content[1]/description[1]/para[2]/internalRef[1]">
    <internalRef internalRefId="stp-0001"
    internalRefTargetType="irtt08"/>
    </object>
    </error>
    </brex>
    </document>
    </brexCheck>
