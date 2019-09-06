NAME
====

s1kd-aspp - Applicability statement preprocessor

SYNOPSIS
========

    s1kd-aspp -h?
    s1kd-aspp -D
    s1kd-aspp -g [-A <ACT>] [-C <CCT>] [-d <dir>] [-F <fmt>] [-G <XSL>]
                 [-cfklNrv] [<object>...]
    s1kd-aspp -p [-a <ID>] [-flv] [<object>...]

DESCRIPTION
===========

The *s1kd-aspp* tool has two main functions:

-   Generates display text for applicability statements. The text is
    derived from the logic described by the `assert` and `evaluate`
    elements.

-   Preprocesses "semantic" applicability statements in a data module to
    produce "presentation" applicability statements which are simpler to
    parse in an XSLT stylesheet.

"Semantic" applicability statements are those entered by the author to
encode the applicability of elements within a data module.
"Presentation" applicability statements are those that are actually
displayed in page-oriented output, also referred to as the
"human-readable" statements.

The applicability in the resulting XML is longer semantically correct,
but an XSLT stylesheet can simply place a statement on any element with
attribute `applicRefId` without needing to consider inherited
applicability statements on elements without the attribute.

OPTIONS
=======

-A, --act &lt;ACT&gt;  
Add an ACT to use when generating display text for product attributes.
Multiple ACT data modules can be used by specifying this option multiple
times.

-a, --id &lt;ID&gt;  
The ID to use for the inline applicability annotation representing the
whole data module's applicability. Default is "app-0000".

-C, --cct &lt;CCT&gt;  
Add a CCT to use when generating display text for conditions. Multiple
CCT data modules can be used by specifying this option multiple times.

-c, --search  
Search for the ACT and CCT referenced by each data module, and add them
to the list of ACTs/CCTs to use when generating display text for that
data module.

-D, --dump  
Dump the built-in XSLT used to generate display text for applicability
statements.

-d, --dir &lt;dir&gt;  
Directory to start searching for ACT/CCT data modules in. By default,
the current directory is used.

-F, --format &lt;fmt&gt;  
Use a custom format string to generate display text.

-f, --overwrite  
Overwrite input data module(s) rather than outputting to stdout.

-G, --xsl &lt;XSLT&gt;  
Use custom XSLT to generate display text for applicability statements.

-g, --generate  
Generate display text for applicability statements.

-h, -?, --help  
Show help/usage message.

-k, --keep  
When generating display text, do not overwrite existing display text on
statements, only generate display text for statements which have none.

-l, --list  
Treat input (stdin or arguments) as lists of filenames of objects,
rather than objects themselves.

-N, --omit-issue  
Assume that the filenames for the ACT and CCT do not include issue info,
i.e. they were created using the -N option of the s1kd-newdm tool.

-p, --presentation  
Preprocess applicability statements to produce "presentation"
applicability statements which are simpler to parse in an XSLT
stylesheet. The applicability in the resulting XML is no longer
semantically correct.

-r, --recursive  
Search for ACT/CCT data modules recursively.

-v, --verbose  
Verbose output.

--version  
Show version information.

&lt;object&gt;...  
The object(s) to preprocess. This can include both individual objects
and combined files such as those produced by s1kd-flatten(1).

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--xinclude  
Do XInclude processing.

EXAMPLES
========

Generating display text
-----------------------

The built-in XSLT for generating display text follows the guidance in
Chap 7.8 of the S1000D 5.0 specification. For example, given the
following:

    <applic>
    <assert applicPropertyIdent="prodversion"
    applicPropertyType="prodattr" applicPropertyValues="A"/>
    </applic>

The resulting XML would contain:

    <applic>
    <displayText>
    <simplePara>prodversion: A</simplePara>
    </displayText>
    <assert applicPropertyIdent="prodversion"
    applicPropertyType="prodattr" applicPropertyValues="A"/>
    </applic>

If ACTs or CCTs are supplied which define display names for a property,
this will be used instead of the ident. For example, the ACT defines the
display name for the "`prodversion`" product attribute:

    <productAttribute id="prodversion">
    <name>Product version</name>
    <displayName>Version</displayName>
    <descr>The version of the product.</descr>
    <enumeration applicPropertyValues="A|B|C"/>
    </productAttribute>

When supplied with the -A option:

    $ s1kd-aspp -g -A <ACT> <DM>

The resulting XML would instead contain:

    <applic>
    <displayText>
    <simplePara>Version: A</simplePara>
    <assert applicPropertyIdent="prodversion"
    applicPropertyType="prodattr" applicPropertyValues="A"/>
    </displayText>
    </applic>

The methods for generating display text can be changed by supplying a
custom XSLT script with the -G option. The -D option can be used to dump
the built-in XSLT as a starting point for a custom script. An identity
template is automatically added to the script, equivalent to the
following:

    <xsl:template match="@*|node()">
    <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    </xsl:template>

This means any elements or attributes not matched by a more specific
template in the script are copied.

Display text format string (-F)
-------------------------------

The -F option allows for simple customizations to generated display text
without needing to create a custom XSLT script (-G). The string
determines the format of the display text of each `<assert>` element in
the annotation.

The following variables can be used within the format string:

%name%  
The name of the property.

%values%  
The applicable value(s) of the property.

For example:

    $ s1kd-aspp -g <DM>
    ...
    <applic>
    <displayText>
    <simplePara>Version: A</simplePara>
    </displayText>
    <assert applicPropertyIdent="version" applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    </applic>
    ...

    $ s1kd-aspp -F '%name% = %values%' -g <DM>
    ...
    <applic>
    <displayText>
    <simplePara>Version = A</simplePara>
    </displayText>
    <assert applicPropertyIdent="version" applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    </applic>
    ...

Creating presentation applicability statements
----------------------------------------------

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
    <applic id="app-B">
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
    <proceduralStep applicRefId="app-B">
    <para>This step is applicable to B only.</para>
    </proceduralStep>
    <proceduralStep applicRefId="app-B">
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

Applicability statements should be displayed whenever applicability
changes:

1.  This step is applicable to A or B.

2.  *Applicable to: B*

    This step is applicable to B only.

3.  This step is also applicable to B only.

4.  *Applicable to: A or B*

    This step is also applicable to A or B.

There are two parts which are difficult to do in an XSLT stylesheet:

-   No statement is shown on Step 3 despite having attribute
    `applicRefId` because the applicability has not changed since the
    last statement on Step 2.

-   A statement is shown on Step 4 despite not having attribute
    `applicRefId` because the applicability has changed back to that of
    the whole data module.

Using the s1kd-aspp tool, the above XML would produce the following
output:

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
    <applic id="app-B">
    <displayText>
    <simplePara>B</simplePara>
    </displayText>
    </applic>
    <applic id="app-0000">
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
    <proceduralStep applicRefId="app-B">
    <para>This step is applicable to B only.</para>
    </proceduralStep>
    <proceduralStep>
    <para>This step is also applicable to B only.</para>
    </proceduralStep>
    <proceduralStep applicRefId="app-0000">
    <para>This step is also applicable to A or B.</para>
    </proceduralStep>
    </mainProcedure>
    </procedure>
    </content>
    </dmodule>

With attribute `applicRefId` only on those elements where a statement
should be shown, and an additional inline applicability to represent the
whole data module's applicability. This XML is semantically incorrect
but easier for a stylesheet to transform for page-oriented output.
