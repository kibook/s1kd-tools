NAME
====

s1kd-brexcheck - Validate S1000D CSDB objects against BREX data modules

SYNOPSIS
========

    s1kd-brexcheck [-b <brex>] [-I <path>] [-w <severities>]
                   [-BcfLlpqS[tu]svxh?] [<object>...]

DESCRIPTION
===========

The *s1kd-brexcheck* tool validates S1000D CSDB objects using the
context, SNS, and/or notation rules of one or multiple BREX (Business
Rules EXchange) data modules. All errors are displayed with the
&lt;objectUse&gt; message, the line number, and a representation of the
invalid XML tree.

OPTIONS
=======

-B  
Check each input object against the appropriate built-in S1000D default
BREX only. The actual BREX reference of each object is ignored.

-b &lt;brex&gt;  
Check the CSDB objects against this BREX. Multiple BREX data modules can
be specified by adding this option multiple times. When no BREX data
modules are specified, the BREX data module referenced in
&lt;brexDmRef&gt; in the CSDB object is attempted to be used instead.

-c  
When a context rule defines values for an object (objectValue), check if
the value of each object is within the allowed set of values.

-f  
Output only the filenames of CSDB objects with BREX/SNS errors.

-h -?  
Show the help/usage message.

-I &lt;path&gt;  
Add a search path for BREX data modules. By default, only the current
directory is searched.

-L  
Treat input as a list of object filenames to check, rather than an
object itself.

-l  
Use the layered BREX concept. BREX data modules referenced by other BREX
data modules (either specified with -b or referenced by the specified
CSDB objects) will also be checked against.

-n  
Check notation rules. Any notation names listed in any of the BREX data
modules with attribute `allowedNotationFlag` set to "1" or omitted are
considered valid notations. If a notation in a CSDB object is not
present or has `allowedNotationFlag` set to "0", an error will be
returned.

For notations not included but not explicitly excluded, the `objectUse`
of the first inclusion rule will be returned with the error. For
explicitly excluded notations, the `objectUse` of the explicit exclusion
rule is returned.

-p  
Display a progress bar.

-q  
Quiet mode. No output is given, errors are only indicated via the return
code.

-S\[tu\]  
Check SNS (Standard Numbering System) rules. The SNS of each specified
data module is checked against the combination of all SNS rules of all
specified BREX data modules.

-s  
Use shortened, single-line messages to report BREX errors instead of
multiline indented messages.

-v  
Verbose mode. The success or failure of each test is printed explicitly.

-w &lt;severities&gt;  
Specify a list of severity levels for business rules.

-x  
Output an XML report instead of a plain-text one.

--version  
Show version information.

Business rule severity levels (`.brseveritylevels`)
---------------------------------------------------

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

Normal, strict and unstrict SNS check (-S, -St, -Su)
----------------------------------------------------

There are three modes for SNS checking: normal, strict, and unstrict.
The main difference between them is how they handle the optional levels
of an SNS description in the BREX.

-St enables *strict* SNS checking. By default, the normal SNS check (-S)
will assume optional elements snsSubSystem, snsSubSubSystem, and snsAssy
exist with an snsCode of "0" ("00" or "0000" for snsAssy) when their
parent element does not contain any of each. This provides a shorthand,
such that

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

-Su enables *unstrict* SNS checking. The normal SNS check (-S) shorthand
mentioned above only allows SNS codes of "0" to be omitted from the SNS
rules. Using unstrict checking, *any* code used will not produce an
error when the relevant optional elements are omitted. This means that
given the following...

    <snsSystem>
    <snsCode>00</snsCode>
    <snsTitle>General</snsTitle>
    </snsSystem>

...SNS codes of 00-00-0000 through 00-ZZ-ZZZZ are considered valid.

Object value checking (-c)
--------------------------

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

-   `"single"` - A single, exact value.

-   `"range"` - Values given in the S1000D range/set notation, e.g.
    `"a~c"` or `"a|b|c"`.

-   `"pattern"` - A regular expression.

The s1kd-brexcheck tool supports all three types. If the `valueForm`
attribute is omitted, it will assume the value is in the `"single"`
notation.

RETURN VALUE
============

The number of BREX errors encountered is returned in the exit status
code.

EXAMPLE
=======

    $ DMOD=DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    $ BREX=DMC-S1000D-F-04-10-0301-00A-022A-D_001-00_EN-US.XML
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
      BREX: DMC-S1000D-F-04-10-0301-00A-022A-D_001-00_EN-US.XML
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
    <brex path="DMC-S1000D-F-04-10-0301-00A-022A-D_001-00_EN-US.XML">
    <error fail="yes">
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
