NAME
====

s1kd-asp - Applicability statement preprocessor

SYNOPSIS
========

    s1kd-asp [-a <ID>] [-fx] [<modules>...]

DESCRIPTION
===========

The *s1kd-asp* tool preprocesses "semantic" applicability statements in a data module to produce "presentation" applicability statements which are simpler to parse in an XSLT stylesheet.

"Semantic" applicability statements are those entered by the author to encode the applicability of elements within a data module. "Presentation" applicability statements are those that are actually displayed in page-oriented output, also referred to as the "human-readable" statements.

The applicability in the resulting XML is longer semantically correct, but an XSLT stylesheet can simply place a statement on any element with attribute `applicRefId` without needing to consider inherited applicability statements on elements without the attribute.

OPTIONS
=======

-a &lt;ID&gt;  
The ID to use for the inline applicability annotation representing the whole data module's applicability. Default is "applic-0000".

-f  
Overwrite input data module(s) rather than outputting to stdout.

-x  
Process the modules using the XInclude specification.

&lt;modules&gt;...  
The module(s) to preprocess. This can include both individual modules and combined files such as those produced by s1kd-flatpm(1).

EXAMPLE
=======

Given the following:

    <dmodule>
    <identAndStatusSection>
    <dmAddress>...</dmAddress>
    <dmStatus>
    ...
    <applic>
    <displayText>
    <simplePara>A or B</simplePara>
    </displayText>
    </applic>
    ...
    </dmStatus>
    </identAndStatusSection>
    <content>
    <referencedApplicGroup>
    <applic id="applic-B">
    <displayText>
    <simplePara>B</simplePara>
    </displayText>
    </applic>
    </referencedApplicGroup>
    <procedure>
    <preliminaryRqmts>...</preliminaryRqmts>
    <mainProcedure>
    <proceduralStep>
    <para>This step is applicable to A or B.</para>
    </proceduralStep>
    <proceduralStep applicRefId="applic-B">
    <para>This step is applicable to B only.</para>
    </proceduralStep>
    <proceduralStep applicRefId="applic-B">
    <para>This step is also applicable to B only.</para>
    </proceduralStep>
    <proceduralStep>
    <para>This step is also applicable to A or B.</para>
    </proceduralStep>
    </mainProcedure>
    <closeRqmts>...</closeRqmts>
    </procedure>
    </content>
    </dmodule>

Applicability statements should be displayed whenever applicability changes:

1.  This step is applicable to A or B.

2.  *Applicable to: B*

    This step is applicable to B only.

3.  This step is also applicable to B only.

4.  *Applicable to: A or B*

    This step is also applicable to A or B.

There are two parts which are difficult to do in an XSLT stylesheet:

-   No statement is shown on Step 3 despite having attribute `applicRefId` because the applicability has not changed since the last statement on Step 2.

-   A statement is shown on Step 4 despite not having attribute `applicRefId` because the applicability has changed back to that of the whole data module.

Using the s1kd-asp tool, the above XML would produce the following output:

    <dmodule>
    <identAndStatusSection>
    <dmAddress>...</dmAddress>
    <dmStatus>
    ...
    <applic>
    <displayText>
    <simplePara>A or B</simplePara>
    </displayText>
    </applic>
    ...
    </dmStatus>
    </identAndStatusSection>
    <content>
    <referencedApplicGroup>
    <applic id="applic-B">
    <displayText>
    <simplePara>B</simplePara>
    </displayText>
    </applic>
    <applic id="applic-0000">
    <displayText>
    <simplePara>A or B</simplePara>
    </displayText>
    </applic>
    </referencedApplicGroup>
    <procedure>
    <preliminaryRqmts>...</preliminaryRqmts>
    <mainProcedure>
    <proceduralStep>
    <para>This step is applicable to A or B.</para>
    </proceduralStep>
    <proceduralStep applicRefId="applic-B">
    <para>This step is applicable to B only.</para>
    </proceduralStep>
    <proceduralStep>
    <para>This step is also applicable to B only.</para>
    </proceduralStep>
    <proceduralStep applicRefId="applic-0000">
    <para>This step is also applicable to A or B.</para>
    </proceduralStep>
    </mainProcedure>
    </procedure>
    </content>
    </dmodule>

With attribute `applicRefId` only on those elements where a statement should be shown, and an additional inline applicability to represent the whole data module's applicability. This XML is semantically incorrect but easier for a stylesheet to transform for page-oriented output.
