NAME
====

s1kd-instance - Create S1000D data/pub module instances

SYNOPSIS
========

    s1kd-instance [-s <src>] [-e <ext>|-E] [-c <dmc>]
                  [-l <lang>] [-n <iss>] [-I <date>]
                  [-u <sec>] [-o <file>|-O <dir>] [-f]
                  [-t <techName>] [-i <infoName>] [-a|-A]
                  [-Y <text>] [-C <comment>]
                  [-R <CIR> ...] [-r <XSL>] [-S] [-N]
                  [-P <PCT> -p <id>] [-L] [<applic>...]

DESCRIPTION
===========

The *s1kd-instance* tool filters a master S1000D data module on user-supplied applicability definitions, producing a new data module instance with non-applicable elements and (optionally) unused applicability statements removed.

OPTIONS
=======

-s &lt;src&gt;  
The source data module or publication module (default is to read from stdin).

-e &lt;ext&gt;  
Specify an extension on the data module code (DME) or publication module code (PME) for the instance.

-E  
Remove the extension from an instance produced from an already extended data module or publication module.

-c &lt;dmc&gt;  
Specify a new data module code (DMC) or publication module code (PMC) for the instance.

-l &lt;lang&gt;  
Set the language and country of the instance. For example, to create an instance for US English, lang would be "en-US".

-n &lt;iss&gt;  
Set the issue and inwork numbers of the instance. By default, the issue and inwork number are taken from the source.

-I &lt;date&gt;  
Set the issue date of the instance. By default, the issue date is taken from the source.

-u &lt;sec&gt;  
Set the security classification of the instance. An instance may have a lower security classification than the source if classified information is removed for a particular customer.

-o &lt;file&gt;  
Output instance to file instead of stdout.

-O &lt;dir&gt;  
Output instance(s) in dir, automatically naming them based on:

-   the extension specified with -e, and/or

-   the code specified with -c, and/or

-   the language and country specified with -L

The issue information is copied from the source or taken as specified in the -n option.

-f  
Overwrite existing file with same name as the filename generated automatically with -O, if it exists.

-t &lt;techName&gt;  
Give the instance a different techName/pmTitle.

-i &lt;infoName&gt;  
Give the data module instance a different infoName.

-a  
Remove unused applicability annotations but not statements.

-A  
Remove unused applicability annotations and simplify/remove unused applicability statements.

-Y &lt;text&gt;  
Set the applicability for the whole data module/publication module using the user-defined applicability values, using text as the new display text.

-C &lt;comment&gt;  
Add an XML comment to the top of the instance. Useful as another way of identifying a data module/publication module as an instance aside from the source address or extended code, or giving additional information about a particular instance.

-R &lt;CIR&gt; ...  
Use a CIR (Common Information Repository) to resolve external dependencies in the master data module, making the instance data module standalone. Additional CIRs can be used by specifying the -R option multiple times.

Currently supported CIRs:

-   Functional items

-   Controls and indicators

-   Zones

-   Warnings and cautions

-   Applicability

-r &lt;XSL&gt;  
Use a custom XSLT script to resolve CIR dependencies for the last specified CIR.

-S  
Do not include &lt;sourceDmIdent&gt;/&lt;sourcePmIdent&gt;/&lt;repositorySourceDmIdent&gt; in the instance.

-w  
Check the applicability of the whole data module/publication module against the user-defined applicability. If the whole data module/publication module is not applicable, then no instance is created.

-N  
Omit issue/inwork numbers from automatically generated filenames.

-P &lt;PCT&gt;  
PCT (Product Cross-reference Table) file to read product definitions from (-p).

-p &lt;id&gt;  
Product ID of the product to read applicability definitions from, using the specified PCT data module (-P).

-L  
Source (-s or stdin) is a list of data module/publication module filenames to create instances of, rather than a single data module/publication module.

-v  
When -O is used, print the automatically generated file name of the instance.

&lt;applic&gt;...  
Any number of applicability definitions in the form of: &lt;ident&gt;:&lt;type&gt;=&lt;value&gt;

-a vs -A
--------

The -a option will remove applicability annotations (applicRefId) from elements which are deemed to be unambiguously valid (their validity does not rely on applicability values left undefined by the user). The applicability statements themselves however will be untouched.

The -A option will do the above, but will also attempt to simplify unused parts of applicability statements or remove unused applicability statements entirely. It simplifies a statement by removing &lt;assert&gt; elements determined to be either unambiguously valid or invalid given the user-defined values, and removing unneeded &lt;evaluate&gt; elements when they contain only one remaining &lt;assert&gt;.

> **Note**
>
> The -A option may change the *meaning* of certain applicability statements without changing the *display text*. Display text is always left untouched, so using this option may cause display text to be technically incorrect.

Identifying source data module of an instance
---------------------------------------------

The resulting data module instance will contain the element &lt;sourceDmIdent&gt;, which will contain the identification elements of the data module specified with the -s option. Publication module instances will contain the element &lt;sourcePmIdent&gt; instead.

Additionally, the instance will contain an element &lt;repositorySourceDmIdent&gt; for each CIR specified with the -R option.

If the -S option is used, neither the &lt;sourceDmIdent&gt;/&lt;sourcePmIdent&gt; elements or &lt;repositorySourceDmIdent&gt; elements are added. This can be useful when this tool is not used to make an "instance" per se, but more generally to make a data module based on an existing data module.

Instance data module code (-c) vs extension (-e)
------------------------------------------------

When creating a data module instance, the instance should have the same data module code as the master data module, with an added extension code, the DME. However, in cases where a vendor does not support this extension or possibly when this tool is used to create "instances" which will from that point on be maintained as normal standalone data modules, it may be desirable or necessary to change the data module code instead. These two options can be used together as well to give an instance a new DMC as well an extension.

Filtering for multiple values of a single property
--------------------------------------------------

Though not usually the case, it is possible to create an instance which is filtered on multiple values of the same applicabilty property. Given the following:

    <referencedApplicGroup>
    <applic id="apA">
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    </applic>
    <applic id="apB">
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="B"/>
    </applic>
    <applic id="apC">
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="C"/>
    </applic>
    </referencedApplicGroup>
    <!-- ... -->
    <para applicRefId="apA">Applies to A</para>
    <para applicRefId="apB">Applies to B</para>
    <para applicRefId="apC">Applies to C</para>

filtering can be applied such that the instance will be applicable to both A and C, but not B. This is done by specifying a property multiple times in the applicability definition arguments. For example:

    $ s1kd-instance -A -Y "A or C" ... attr:prodattr=A attr:prodattr=C

This would produce the following in the instance:

    <dmStatus>
      <!-- ... -->
    <applic>
    <displayText>
    <simplePara>A or C</simplePara>
    </displayText>
    <evaluate andOr="or">
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="C"/>
    </evaluate>
    </applic>
    <!-- ... ->
    </dmStatus>
    <!-- ... -->
    <referencedApplicGroup>
    <applic id="apA">
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="A"/>
    </applic>
    <applic id="apC">
    <assert applicPropertyIdent="attr"
    applicPropertyType="prodattr"
    applicPropertyValues="C"/>
    </applic>
    </referencedApplicGroup>
    <!-- ... -->
    <para applicRefId="apA">Applies to A</para>
    <para applicRefId="apC">Applies to C</para>

Resolving CIR dependencies with a custom XSLT script (-r)
---------------------------------------------------------

A CIR contains more information about an item than can be captured in a data module's reference to it. If this additional information is required, there are two methods to include it:

-   Distribute the CIR with the data module so the extra information can be linked to

-   "Flatten" the information to fit in the data module's schema.

A custom XSLT script can be supplied with the -r option, which is then used to resolve the CIR dependencies of the last CIR specified with -R. For example:

    <xsl:stylesheet
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    version="1.0">
    <xsl:template match="functionalItemRef">
    <xsl:variable name="fin" select"@functionalItemNumber"/>
    <xsl:variable name="spec" select="$cir//functionalItemSpec[
    functionalItemIdent/@functionalItemNumber = $fin]"/>
    <xsl:value-of select="$spec/name"/>
    </xsl:template>
    </xsl:stylesheet>

This script would resolve a `functionalItemRef` by "flattening" it to the value of the `name` element obtained from the CIR.

The example CIR would contain a specification like:

    <functionalItemSpec>
    <functionalItemIdent functionalItemNumber="ABC" functionalItemType="fit01"/>
    <name>Hydraulic pump</name>
    <functionalItemAlts>
    <functionalItem/>
    </functionalItemAlts>
    </functionalItemSpec>

The source data module would contain a reference:

    <para>The <functionalItemRef functionalItemNumber="ABC"/> is an item in the system.</para>

And the resulting XML would be:

    <para>The Hydraulic pump is an item in the system.</para>

The source data module and CIR are combined in to a single XML document which is used as the input to the XSLT script. The root element `mux` contains two `dmodule` elements. The first is the source data module, and the second is one of the CIR data modules supplied with -R. The CIR data module is first filtered on the defined applicability.
