NAME
====

s1kd-brexcheck - Validate S1000D data modules against BREX data modules

SYNOPSIS
========

s1kd-brexcheck \[-b &lt;brex&gt;\] \[-I &lt;path&gt;\] \[-vVqDsxlh?\] &lt;datamodules&gt;

DESCRIPTION
===========

The *s1kd-brexcheck* tool validates an S1000D data module using the context rules of one or multiple Business Rule EXchange (BREX) data modules. All errors are displayed with the &lt;objectUse&gt; message, the line number, and a representation of the invalid XML tree.

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

-h -?  
Show the help/usage message.

RETURN VALUE
============

The number of BREX errors encountered is returned in the exit status code.

Example
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
