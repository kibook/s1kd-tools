# NAME

s1kd-aspp - Applicability statement preprocessor

# SYNOPSIS

    s1kd-aspp [options] [<object> ...]

# DESCRIPTION

The *s1kd-aspp* tool has two main functions:

  - Generates display text for applicability statements. The text is
    derived from the logic described by the `assert` and `evaluate`
    elements.

  - Preprocesses "semantic" applicability statements in a data module to
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

# OPTIONS

  - \-., --dump-disptext  
    Dump the built-in .disptext file.

  - \-,, --dump-xsl  
    Dump the built-in XSLT used to generate display text for
    applicability statements.

  - \-A, --act \<ACT\>  
    Add an ACT to use when generating display text for product
    attributes. Multiple ACT data modules can be used by specifying this
    option multiple times.

  - \-a, --id \<ID\>  
    The ID to use for the inline applicability annotation representing
    the whole data module's applicability. Default is "app-0000".

  - \-C, --cct \<CCT\>  
    Add a CCT to use when generating display text for conditions.
    Multiple CCT data modules can be used by specifying this option
    multiple times.

  - \-c, --search  
    Search for the ACT and CCT referenced by each data module, and add
    them to the list of ACTs/CCTs to use when generating display text
    for that data module.

  - \-D, --delete  
    Remove the display text from all applicability annotations, except
    those that consist of only display text (and no computer processing
    part).

  - \-d, --dir \<dir\>  
    Directory to start searching for ACT/CCT data modules in. By
    default, the current directory is used.

  - \-F, --format \<fmt\>  
    Use a custom format string to generate display text.

  - \-f, --overwrite  
    Overwrite input data module(s) rather than outputting to stdout.

  - \-G, --disptext \<disptext\>  
    Specify a custom .disptext file.

  - \-g, --generate  
    Generate display text for applicability statements.

  - \-h, -?, --help  
    Show help/usage message.

  - \-k, --keep  
    When generating display text, do not overwrite existing display text
    on statements, only generate display text for statements which have
    none.

  - \-l, --list  
    Treat input (stdin or arguments) as lists of filenames of objects,
    rather than objects themselves.

  - \-N, --omit-issue  
    Assume that the filenames for the ACT and CCT do not include issue
    info, i.e. they were created using the -N option of the s1kd-newdm
    tool.

  - \-p, --presentation  
    Preprocess applicability statements to produce "presentation"
    applicability statements which are simpler to parse in an XSLT
    stylesheet. The applicability in the resulting XML is no longer
    semantically correct.

  - \-r, --recursive  
    Search for ACT/CCT data modules recursively.

  - \-t, --tags \<mode\>  
    Add tags before elements containing the display text of the
    applicability annotation they reference, simulating the typical
    presentation of applicability annotations within the XML.
    
    If \<mode\> is "pi", the tags are inserted as processing
    instructions, named "s1kd-aspp". This allows existing tags to be
    removed automatically before adding new ones.
    
    If \<mode\> is "comment", the tags are inserted as XML comments.
    Existing comments will not be removed automatically.
    
    If \<mode\> is "remove", tags will be removed without adding new
    ones. This only applies to the processing instruction tags.

  - \-v, --verbose  
    Verbose output.

  - \-x, --xsl \<XSLT\>  
    Use custom XSLT to generate display text for applicability
    statements.

  - \--version  
    Show version information.

  - \<object\> ...  
    The object(s) to preprocess. This can include both individual
    objects and combined files such as those produced by
    s1kd-flatten(1).

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

## `.disptext` file

This file specifies rules for generating display text. It consists of:

  - operator rules

  - property rules

The `<operators>` element specifies the format of operators used in
display text:

  - and  
    Text to use for the `and` operator between assertions. Default is "
    and ".

  - or  
    Text to use for the `or` operator between assertions. Default is "
    or ".

  - openGroup  
    Text to use to open a group of assertions. Default is "(".

  - closeGroup  
    Text to use to close a group of assertions. Default is ")".

  - set  
    Text to use between items in a set (a|b|c).

  - range  
    Text to use between the start and end of a range (a\~c).

Each `<property>` element specifies the format used for an individual
property. The `<productAttributes>` and `<conditions>` elements specify
the default format for product attributes and conditions that are not
listed. Alternatively, the `<default>` element specifies the default
format for both product attributes and conditions together.

The format is specified using a combination of the following elements:

  - \<name\>  
    Replaced by the name of the property.

  - \<text\>  
    Text that is included as-is.

  - \<values\>  
    Replaced by the values specified for the property in the
    applicability assertion.

Optionally, `<values>` may contain a list of custom labels for
individual values. Any values not included in this list will use their
normal label.

By default, the program will search for a file named `.disptext` in the
current directory and parent directories, but any file can be specified
using the -G (--disptext) option.

Example of a `.disptext` file:

    <disptext>
    <operators>
    <and> + </and>
    <or>, </or>
    <openGroup>[</openGroup>
    <closeGroup>]</closeGroup>
    <set> or </set>
    <range> thru </range>
    </operators>
    <default>
    <name/>
    <text>: </text>
    <values/>
    </default>
    <property ident="model" type="prodattr">
    <values>
    <value match="BRKTRKR">Brook trekker</value>
    <value match="MNTSTRM">Mountain storm</value>
    </values>
    <text> </text>
    <name/>
    </property>
    </disptext>

Given the above example, the following display would be generated for
each annotation:

Assert annotation:

    <assert
    applicPropertyIdent="model"
    applicPropertyType="prodattr"
    applicPropertyValues="BRKTRKR"/>

Human-readable format:

    "Brook trekker Model"

Evaluate annotation:

    <evaluate andOr="or">
    <evaluate andOr="and">
    <assert
    applicPropertyIdent="model"
    applicPropertyType="prodattr"
    applicPropertyValues="BRKTRKR"/>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="Mk1"/>
    </evaluate>
    <evaluate andOr="and">
    <assert
    applicPropertyIdent="model"
    applicPropertyType="prodattr"
    applicPropertyValues="MNTSTRM"/>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="Mk9"/>
    </evaluate>
    </evaluate>

Human-readable format:

    "[Brook trekker Model + Version: Mk9],
    [Mountain storm Model + Version: Mk1]"

Evaluate annotation:

    <evaluate andOr="and">
    <assert
    applicPropertyIdent="model"
    applicPropertyType="prodattr"
    applicPropertyValues="BRKTRKR|MNTSTRM"/>
    <assert
    applicPropertyIdent="version"
    applicPropertyType="prodattr"
    applicPropertyValues="Mk1~Mk9"/>
    </evaluate>

Human-readable format:

    "Brook trekker or Mountain storm Model + Version: Mk1 thru Mk9"

# EXAMPLES

## Generating display text

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

The methods for generating display text can be changed either via the
`.disptext` file, or by supplying a custom XSLT script with the -x
option. The -, option can be used to dump the built-in XSLT as a
starting point for a custom script.

## Display text format string (-F)

The -F option allows for very simple customizations to generated display
text without needing to create a custom `.disptext` file or XSLT script
(-x). The string determines the format of the display text of each
`<assert>` element in the annotation.

The following variables can be used within the format string:

  - %name%  
    The name of the property.

  - %values%  
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

## Creating presentation applicability statements

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

  - No statement is shown on Step 3 despite having attribute
    `applicRefId` because the applicability has not changed since the
    last statement on Step 2.

  - A statement is shown on Step 4 despite not having attribute
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

# DISPTEXT FILE SCHEMA

## Display text rules

The element `<disptext>` contains all the rules for the formatting of
generated display text in applicability annotations.

*Markup element:* `<disptext>`

*Attributes:*

  - None

*Child elements:*

  - `<operators>`

  - `<default>`

  - `<productAttributes>`

  - `<conditions>`

  - `<conditionType>`

  - `<property>`

## Operator rules

The element `<operators>` defines the format of operators used in
applicability display text.

*Markup element:* `<operators>`

*Attributes:*

  - None

*Child elements:*

  - `<and>`, text used for the `and` operator between assertions in an
    evaluation.

  - `<or>`, text used for the `or` operator between assertions in an
    evaluation.

  - `<openGroup>`, text used to open a group of assertions.

  - `<closeGroup>`, text used to close a group of assertions.

  - `<set>`, text used between items in a set.

  - `<range>`, text used between the start and end of a range.

## Default property format

The element `<default>` defines the default format for all properties
which are not matched by a more specific rule.

*Markup element:* `<default>`

*Attributes:*

  - None

*Child elements:*

  - `<name>`, replaced by the name of the property.

  - `<text>`, text that is included as-is.

  - `<values>`, replaced by the values specified for the property in the
    applicability assertion.

## Product attributes format

The element `<productAttributes>` defines the default format for all
product attributes which are not matched by a more specific rule.

*Markup element:* `<productAttributes>`

*Attributes:*

  - None

*Child elements:*

  - `<name>`, replaced by the name of the product attribute.

  - `<text>`, text that is included as-is.

  - `<values>`, replaced by the values specified for the product
    attribute in the applicability assertion.

## Conditions format

The element `<conditions>` defines the default format for all conditions
which are not matched by a more specific rule.

*Markup element:* `<conditions>`

*Attributes:*

  - None

*Child elements:*

  - `<name>`, replaced by the name of the condition.

  - `<text>`, text that is included as-is.

  - `<values>`, replaced by the values specified for the condition in
    the applicability assertion.

## Condition type format

The element `<conditionType>` defines the format for all conditions of a
given type which are not matched by a more specific rule.

*Markup element:* `<conditionType>`

*Attributes:*

  - `ident` (M), the ID of the condition type in the CCT.

*Child elements:*

  - `<name>`, replaced by the name of the condition.

  - `<text>`, text that is included as-is.

  - `<values>`, replaced by the values specified for the condition in
    the applicability assertion.

## Property format

The element `<property>` defines the format for a specific property.

*Markup element:* `<property>`

*Attributes:*

  - `ident` (M), the ID of the property in the ACT or CCT.

  - `type` (M), the type of the property, either "`condition`" or
    "`prodattr`".

*Child elements:*

  - `<name>`, replaced by the name of the property.

  - `<text>`, text that is included as-is.

  - `<values>`, replaced by the values specified for the property in the
    applicability assertion.

## Values

The element `<values>` is replaced by the values specified for a
property in an applicability assertion, and may specify custom labels
for certain values.

*Markup element:* `<values>`

*Attributes:*

  - None

*Child elements:*

  - `<value>`

## Custom value label

The element `<value>` specifies a custom label for an individual value
of a property.

*Markup element:* `<value>`

*Attributes:*

  - `match` (M), the value to apply the custom label for.

*Child elements:*

  - None
