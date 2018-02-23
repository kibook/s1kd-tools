NAME
====

s1kd-newdm - Create a new S1000D data module

SYNOPSIS
========

    s1kd-newdm [options]

DESCRIPTION
===========

The *s1kd-newdm* tool creates a new S1000D data module with the data module code and other metadata specified.

OPTIONS
=======

-\# &lt;DMC&gt;  
The data module code of the new data module.

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-@ &lt;filename&gt;  
Save the new data module as &lt;filename&gt; instead of an automatically named file in the current directory.

-% &lt;dir&gt;  
Use XML templates in the specified directory instead of the built-in templates.

-,  
Dumps the built-in default 'dmtypes' XML. This can be used to quickly set up a starting point for a project's custom info codes, from which info names can be modified and unused codes can be removed to fit the project.

-.  
Dumps the simple text form of the built-in default 'dmtypes'.

-b &lt;BREX&gt;  
BREX data module code.

-C &lt;country&gt;  
The country ISO code of the new data module.

-c &lt;sec&gt;  
The security classification of the new data module.

-D &lt;dmtypes&gt;  
Specify the 'dmtypes' file name.

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-f  
Overwrite existing file.

-I &lt;date&gt;  
Issue date of the new data module in the form of YYYY-MM-DD.

-i &lt;info&gt;  
The info name of the new data module.

-L &lt;language&gt;  
The language ISO code of the new data module.

-m &lt;remarks&gt;  
Set remarks for the new data module.

-N  
Omit issue/inwork numbers from filename.

-n &lt;issue&gt;  
The issue number of the new data module.

-O &lt;CAGE&gt;  
The CAGE code of the originator.

-o &lt;orig&gt;  
The originator enterprise name of the new data module.

-p  
Prompts the user for any values left unspecified.

-R &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r &lt;RPC&gt;  
The responsible partner company enterprise name of the new data module.

-S &lt;BREX&gt;  
Determine the tech name from the SNS rules of a specified BREX data module. This can also be specified in the 'defaults' file with the key 'sns'.

-s &lt;schema&gt;  
The schema URL.

-T &lt;schema&gt;  
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

-t &lt;tech&gt;  
The tech name of the new data module.

-v  
Print the file name of the newly created data module.

-w &lt;inwork&gt;  
The inwork number of the new data module.

Prompt (-p) option
------------------

If this option is specified, the program will prompt the user to enter values for metadata which was not specified when calling the program. If a piece of metadata has a default value (from the 'defaults' and 'dmtypes' files), it will be displayed in square brackets \[\] in the prompt, and pressing Enter without typing any value will select this default value.

'defaults' file
---------------

This file sets default values for each piece of metadata. By default, the program will search the current directory for a file named 'defaults', but any file can be specified by using the -d option.

All of the s1kd-new\* commands use the same 'defaults' file format, so this file can contain default values for multiple types of metadata.

Each line consists of the identifier of a piece of metadata and its default value, separated by whitespace. Lines which do not match a piece of metadata are ignored, and may be used as comments. Example:

    # General
    modelIdentCode            S1000DTOOLS
    securityClassification    01
    responsiblePartnerCompany khzae.net
    originator                khzae.net
    languageIsoCode           en
    countryIsoCode            CA
    issueNumber               000
    inWork                    01

    # Data modules
    systemDiffCode            A
    systemCode                00
    subSystemCode             0
    subSubSystemCode          0
    assyCode                  00
    disassyCode               00
    disassyCodeVariant        A
    infoCode                  040
    infoCodeVariant           A
    itemLocationCode          D

    # Comments/DDN
    senderIdent               KHZAE
    yearOfDataIssue           2017
    seqNumber                 00001
    city                      Toronto
    country                   Canada

    # Comments
    commentType               q
    commentPriorityCode       cp01

    # DDN
    authorization             khzae.net

    # Publication modules
    pmIssuer                  KHZAE
    pmNumber                  00001
    pmVolume                  00

Alternatively, the 'defaults' file can be written using an XML format, containing a root element `defaults` with child elements `default` which each have an attribute `ident` and an attribute `value`.

    <?xml version="1.0"?>
    <defaults>
    <!-- General -->
    <default ident="modelIdentCode" value="S1000DTOOLS"/>
    <default ident="securityClassification" value="01"/>
    [...]
    </defaults>

'dmtypes' file
--------------

This file sets the default type (schema) for data modules based on their info code. By default, the program will search the current directory for a file named 'dmtypes', but any file can be specified by using the -D option.

Each line consists of an info code, a schema identifier, and optionally a default info name. Example:

    00E    comrep
    00W    appliccrossreftable
    009    frontmatter
    022    brex
    024    brdoc
    040    descript    Description
    520    proced      Remove procedure

Like the 'defaults' file, the 'dmtypes' file may also be written in an XML format, where each child has an attribute `infoCode` and an attribute `schema`.

    <?xml version="1.0">
    <dmtypes>
    <type infoCode="022" schema="brex"/>
    <type infoCode="040" schema="descript" infoName="Description"/>
    <type infoCode="520" schema="proced" infoName="Remove procedure"/>
    </dmtypes>

Info code variants can also be given specific default schema and info names. To do this, include the variant with the info code:

    258A  proced  Other procedure to clean
    258B  proced  Other procedure to clean, Clean with air
    258C  proced  Other procedure to clean, Clean with water

The two forms of info codes (with and without variant) can be mixed. Defaults are chosen in the order they are listed in the 'dmtypes' file. An info code with no variant matches all possible variants.

Custom XML templates (-%)
-------------------------

A minimal set of S1000D templates are built-in to this tool, but customized templates may be used with the -% option. This option takes a path to a directory where the custom templates are located. Each template should be named `<schema>.xml`, where `<schema>` is the name of the schema, matching one of the schema names in the 'dmtypes' file or the schema specified with the -T option.

The templates must be written to conform to the default S1000D issue of this tool (currently 4.2). They will be automatically transformed when another issue is specified with the -$ option.

The 'templates' default can also be specified in the 'defaults' file to use these custom templates by default.

EXAMPLE
=======

    $ s1kd-newdm -# S1000DTOOLS-A-00-07-00-00A-040A-D
