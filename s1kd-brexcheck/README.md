NAME
====

s1kd-brexcheck - Validate S1000D data modules against BREX data modules

SYNOPSIS
========

s1kd-brexcheck \[-b &lt;brex&gt;\] \[-I &lt;path&gt;\] \[-w &lt;severities&gt;\] \[-vVqDsxlStuh?\] &lt;datamodules&gt;

DESCRIPTION
===========

The *s1kd-brexcheck* tool validates an S1000D data module using the context rules of one or multiple BREX (Business Rules EXchange) data modules. All errors are displayed with the &lt;objectUse&gt; message, the line number, and a representation of the invalid XML tree.

OPTIONS
=======

-b &lt;brex&gt;  
Check the data modules against this BREX. Multiple BREX data modules can be specified by adding this option multiple times. When no BREX data modules are specified, the BREX data module referenced in &lt;brexDmRef&gt; in the data module is attempted to be used instead.

-I &lt;path&gt;  
Add a search path for BREX data modules. By default, only the current directory is searched.

-v -V -q -D  
Verbosity of the output.

-s  
Use shortened, single-line messages to report BREX errors instead of multiline indented messages.

-x  
Output an XML report instead of a plain-text one.

-l  
Use the layered BREX concept. BREX data modules referenced by other BREX data modules (either specified with -b or referenced by the specified data modules) will also be checked against.

-w &lt;severities&gt;  
Specify a list of severity levels for business rules.

-S\[tu\]  
Check SNS (Standard Numbering System) rules. The SNS of each specified data module is checked against the combination of all SNS rules of all specified BREX data modules.

-St enables *strict* SNS checking. By default, the normal SNS check (-S) will assume optional elements snsSubSystem, snsSubSubSystem, and snsAssy exist with an snsCode of "0" ("00" or "0000" for snsAssy) when their parent element does not contain any of each. This provides a shorthand, such that

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

Using strict checking will disable this shorthand, and missing optional elements will result in an error.

-Su enables *unstrict* SNS checking. The normal SNS check (-S) shorthand mentioned above only allows SNS codes of "0" to be omitted from the SNS rules. Using unstrict checking, *any* code used will not produce an error when the relevant optional elements are omitted. This means that given the following...

    <snsSystem>
      <snsCode>00</snsCode>
      <snsTitle>General</snsTitle>
    </snsSystem>

...SNS codes of 00-00-0000 through 00-ZZ-ZZZZ are considered valid.

-p  
Display a progress bar.

-f  
Output only the filenames if modules with BREX/SNS errors.

-h -?  
Show the help/usage message.

Business rule severity levels (-w)
----------------------------------

The attribute brSeverityLevel on a BREX rule allows for distinguishing different kinds of errors. The -w option takes an XML file containing a list of severity levels, their user-defined type, and optionally if they should not be counted as true errors (causing the tool to return a "failure" status) but merely warnings.

An example of the format of this file is given below:

    <?xml version="1.0"?>
    <brSeverityLevels>
      <brSeverityLevel value="brsl01" fail="yes">Error</brSeverityLevel>
      <brSeverityLevel value="brsl02" fail="no">Warning</brSeverityLevel>
    </brSeverityLevels>

When the attribute `fail` has a value of "yes" (or is not included), BREX errors pertaining to rules with the given severity level value will be counted as errors. When it is no, the errors are still displayed but are not counted as errors in the exit status code of the tool.

RETURN VALUE
============

The number of BREX errors encountered is returned in the exit status code.

EXAMPLE
=======

    $ DMOD=DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    $ BREX=DMC-S1000D-F-04-10-0301-00A-022A-D_001-00_EN-US.XML
    $ cat $DMOD
    [...]
    <listItem id="stp-0001">
      <para>List items shouldn't be used as steps...</para>
    </listItem>
    [...]
    <para>Refer to <internalRef internalRefId="stp-0001" internalRefTargetType="irtt08"/>.</para>
    [...]

    $ s1kd-brexcheck -b $BREX $DMOD
    BREX ERROR: DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
      Only when the reference target is a step can the value of attribute
    internalRefTargetType be irtt08 (Chap 3.9.5.2.1.2, Para 2.1).
      line 53:
        ELEMENT internalRef
          ATTRIBUTE internalRefId
            TEXT
              content=stp-0001
          ATTRIBUTE internalRefTargetType
            TEXT
              content=irtt08
