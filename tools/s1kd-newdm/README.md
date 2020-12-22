NAME
====

s1kd-newdm - Create a new S1000D data module

SYNOPSIS
========

    s1kd-newdm [options]

DESCRIPTION
===========

The *s1kd-newdm* tool creates a new S1000D data module with the data
module code and other metadata specified.

OPTIONS
=======

-\#, --code &lt;DMC&gt;  
The data module code of the new data module. The prefix "DMC-" is
optional.

If - is given for the code, a random data module code will be generated.
If only a model identification code is given instead (e.g., `-# TEST`),
or the `.defaults` file specifies a default model identification code,
this will be used as part of the random code. The information type of
the random code will be 000A-D.

-$, --issue &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   5.0 (default)

-   4.2

-   4.1

-   4.0

-   3.0

-   2.3

-   2.2

-   2.1

-   2.0

-@, --out &lt;path&gt;  
Save the new data module to &lt;path&gt;. If &lt;path&gt; is an existing
directory, the data module will be created in it instead of the current
directory. Otherwise, the data module will be saved as the filename
&lt;path&gt; instead of being automatically named.

-%, --templates &lt;dir&gt;  
Use XML templates in the specified directory instead of the built-in
templates.

-\~, --dump-templates &lt;dir&gt;  
Dump the built-in XML templates to the specified directory.

-,, --dump-dmtypes-xml  
Dumps the built-in default `.dmtypes` XML. This can be used to quickly
set up a starting point for a project's custom info codes, from which
info names can be modified and unused codes can be removed to fit the
project.

-., --dump-dmtypes  
Dumps the simple text form of the built-in default `.dmtypes`.

-!, --no-infoname  
Do not include an info name for the new data module.

-a, --act &lt;ACT&gt;  
ACT data module code.

-B, --generate-brex-rules  
When creating a new BREX data module, use the `.defaults` and `.dmtypes`
files to add a basic set of context rules.

-b, --brex &lt;BREX&gt;  
BREX data module code.

-C, --country &lt;country&gt;  
The country ISO code of the new data module.

-c, --security &lt;sec&gt;  
The security classification of the new data module.

-D, --dmtypes &lt;dmtypes&gt;  
Specify the `.dmtypes` file name.

-d, --defaults &lt;defaults&gt;  
Specify the `.defaults` file name.

-f, --overwrite  
Overwrite existing file.

-h, -?, --help  
Show help/usage message.

-I, --date &lt;date&gt;  
Issue date of the new data module in the form of YYYY-MM-DD.

-i, --infoname &lt;info&gt;  
The info name of the new data module.

-j, --brexmap &lt;map&gt;  
Use a custom `.brexmap` file when using the -B option.

-k, --skill &lt;skill&gt;  
The skill level code of the new data module.

-L, --language &lt;language&gt;  
The language ISO code of the new data module.

-M, --maintained-sns &lt;SNS&gt;  
Determine the tech name from on one of the built-in S1000D maintained
SNS. Supported SNS:

-   Generic

-   Support and training equipment

-   Ordnance

-   General communications

-   Air vehicle, engines and equipment

-   Tactical missiles

-   General surface vehicles

-   General sea vehicles

When creating a BREX data module, this SNS will be included as the SNS
rules of the new data module. The "`maintainedSns`" `.defaults` file key
can be used to set one of the above SNS as the default.

-m, --remarks &lt;remarks&gt;  
Set remarks for the new data module.

-N, --omit-issue  
Omit issue/inwork numbers from filename.

-n, --issno &lt;issue&gt;  
The issue number of the new data module.

-O, --origcode &lt;CAGE&gt;  
The CAGE code of the originator.

-o, --origname &lt;orig&gt;  
The originator enterprise name of the new data module.

-P, --sns-levels &lt;levels&gt;  
When determining tech name from an SNS (-S or -M), include the specified
number of levels of SNS in the tech name, from 1 (default) to 4. Each
level is separated by " - ".

For example, if &lt;levels&gt; is 2, then:

-   tech names derived from a subsystem will be formatted as "System -
    Subsystem"

-   tech names derived from a subsubsystem will be formatted as
    "Subsystem - Subsubsystem"

-   and tech names derived from an assembly will be formatted as
    "Subsubsystem - Assembly".

If two levels have the same title, then only one will be used. The
"`snsLevels`" `.defaults` file key can also be set to control this
option.

-p, --prompt  
Prompts the user for any values left unspecified.

-q, --quiet  
Do not report an error when the file already exists.

-R, --rpccode &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r, --rpcname &lt;RPC&gt;  
The responsible partner company enterprise name of the new data module.

-S, --sns &lt;BREX&gt;  
Determine the tech name from the SNS rules of a specified BREX data
module. This can also be specified in the `.defaults` file with the key
"`sns`", or the key "`brex`" if "`sns`" is not specified.

-s, --schema &lt;schema&gt;  
The schema URL.

-T, --type &lt;schema&gt;  
The type (schema) of the new data module. Supported schemas:

-   appliccrossreftable - Applicability cross-reference table

-   brdoc - Business rule document

-   brex - Business rule exchange

-   checklist - Maintenance checklist

-   comrep - Common information repository

-   condcrossreftable - Conditions cross-reference table

-   container - Container

-   crew - Crew/Operator information

-   descript - Descriptive

-   fault - Fault information

-   frontmatter - Front matter

-   ipd - Illustrated parts data

-   learning - Technical training information

-   prdcrossreftable - Product cross-reference table

-   proced - Procedural

-   process - Process

-   sb - Service bulletin

-   schedul - Maintenance planning information

-   scocontent - SCO content information

-   techrep - Technical repository (replaced by comrep in issue 4.1)

-   wrngdata - Wiring data

-   wrngflds - Wiring fields

-t, --techname &lt;tech&gt;  
The tech name of the new data module.

-V, --infoname-variant &lt;variant&gt;  
The info name variant of the new data module.

-v, --verbose  
Print the file name of the newly created data module.

-w, --inwork &lt;inwork&gt;  
The inwork number of the new data module.

-z, --issue-type &lt;type&gt;  
The issue type of the new data module.

--version  
Show version information.

In addition, the following options allow configuration of the XML
parser:

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

--xml-catalog &lt;file&gt;  
Use an XML catalog when resolving entities. Multiple catalogs may be
loaded by specifying this option multiple times.

Prompt (-p) option
------------------

If this option is specified, the program will prompt the user to enter
values for metadata which was not specified when calling the program. If
a piece of metadata has a default value (from the `.defaults` and
`.dmtypes` files), it will be displayed in square brackets \[\] in the
prompt, and pressing Enter without typing any value will select this
default value.

`.defaults` file
----------------

This file sets default values for each piece of metadata. By default,
the program will search the current directory and parent directories for
a file named `.defaults`, but any file can be specified by using the -d
option.

All of the s1kd-new\* commands use the same `.defaults` file format, so
this file can contain default values for multiple types of metadata.

Each line consists of the identifier of a piece of metadata and its
default value, separated by whitespace. Lines which do not match a piece
of metadata are ignored, and may be used as comments. Example:

    # General
    countryIsoCode               CA
    languageIsoCode              en
    originator                   khzae.net
    responsiblePartnerCompany    khzae.net
    securityClassification       01

Alternatively, the `.defaults` file can be written using an XML format,
containing a root element `defaults` with child elements `default` which
each have an attribute `ident` and an attribute `value`.

    <?xml version="1.0"?>
    <defaults>
    <!-- General -->
    <default ident="countryIsoCode" value="CA"/>
    <default ident="languageIsoCode" value="en"/>
    <default ident="originator" value="khzae.net"/>
    <default ident="responsiblePartnerCompany" value="khzae.net"/>
    <default ident="securityClassification" value="01"/>
    </defaults>

`.dmtypes` file
---------------

This file sets the default schema and info name for data modules based
on their info code. By default, the program will search the current
directory and parent directories for a file named `.dmtypes`, but any
file can be specified by using the -D option.

Each line consists of an info code, a schema identifier, and optionally
a default info name. Example:

    000    descript
    022    brex        Business rules
    040    descript    Description
    520    proced      Remove procedure

Like the `.defaults` file, the `.dmtypes` file may also be written in an
XML format, where each child has an attribute `infoCode`, an attribute
`schema`, and optionally an attribute `infoName`.

    <?xml version="1.0">
    <dmtypes>
    <type infoCode="000" schema="descript"/>
    <type infoCode="022" schema="brex" infoName="Business rules"/>
    <type infoCode="040" schema="descript" infoName="Description"/>
    <type infoCode="520" schema="proced" infoName="Remove procedure"/>
    </dmtypes>

The info code field can also include an info code variant, item location
code, learn code, and learn event code, which allows for more specific
default schemas and info names.

Example of info code variants:

    258A  proced  Other procedure to clean
    258B  proced  Other procedure to clean, Clean with air
    258C  proced  Other procedure to clean, Clean with water

Example of item location codes:

    200A-A  proced  Servicing, while installed
    200A-C  proced  Servicing, on the bench
    200A-T  proced  Servicing, training

Example of learn codes:

    100A-A-H10A  learning  Operation: Performance analysis
    100A-A-T5CC  learning  Operation: Simulation
    100A-A-T80E  learning  Operation: Assessment

The XML format additionally supports the use of the attribute
`infoNameVariant`, for use with S1000D Issue 5.0 and up, allowing
extensions of the info name to be encoded separately:

    <dmtypes>
    <type
    infoCode="258A"
    schema="proced"
    infoName="Other procedure to clean"/>
    <type
    infoCode="258B"
    schema="proced"
    infoName="Other procedure to clean"
    infoNameVariant="Clean with air"/>
    <type
    infoCode="258C"
    schema="proced"
    infoName="Other procedure to clean"
    infoNameVariant="Clean with water"/>
    </dmtypes>

Defaults are chosen in the order they are listed in the `.dmtypes` file.
An info code which does not specify a variant, item location code, learn
code or learn event code, or uses asterisks in their place, matches all
possible variants, item location codes, learn codes and learn event
codes.

`.brexmap` file
---------------

Refer to the documentation for s1kd-defaults(1) for a description of the
`.brexmap` file.

Custom XML templates (-%)
-------------------------

A minimal set of S1000D templates are built-in to this tool, but
customized templates may be used with the -% option. This option takes a
path to a directory where the custom templates are located. Each
template should be named `<schema>.xml`, where `<schema>` is the name of
the schema, matching one of the schema names in the `.dmtypes` file or
the schema specified with the -T option.

The templates must be written to conform to the default S1000D issue of
this tool (currently 5.0), regardless of what issue of S1000D the
project is using. The final output will be automatically transformed
when another issue is specified with the -$ option.

The `templates` default can also be specified in the `.defaults` file to
use these custom templates by default.

EXAMPLE
=======

    $ s1kd-newdm -# S1KDTOOLS-A-00-07-00-00A-040A-D
