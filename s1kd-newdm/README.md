NAME
====

s1kd-newdm - Create a new S1000D data module

SYNOPSIS
========

s1kd-newdm \[options\]

DESCRIPTION
===========

The *s1kd-newdm* tool creates a new S1000D data module with the data module code and other metadata specified.

OPTIONS
=======

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-D &lt;dmtypes&gt;  
Specify the 'dmtypes' file name.

-p  
Prompts the user for any values left unspecified.

-\# &lt;DMC&gt;  
The data module code of the new data module.

-L &lt;language&gt;  
The language ISO code of the new data module.

-C &lt;country&gt;  
The country ISO code of the new data module.

-n &lt;issue&gt;  
The issue number of the new data module.

-w &lt;inwork&gt;  
The inwork number of the new data module.

-c &lt;sec&gt;  
The security classification of the new data module.

-r &lt;RPC&gt;  
The responsible partner company enterprise name of the new data module.

-R &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-o &lt;orig&gt;  
The originator enterprise name of the new data module.

-O &lt;CAGE&gt;  
The CAGE code of the originator.

-t &lt;tech&gt;  
The tech name of the new data module.

-i &lt;info&gt;  
The info name of the new data module.

-T &lt;schema&gt;  
The type (schema) of the new data module. Supported schemas:

-   appliccrossreftable - Applicability cross-reference table

-   brdoc - Business rule document

-   brex - Business rule exchange

-   checklist - Maintenance checklist

-   comrep - Common information repository

-   condcrossreftable - Conditions cross-reference table

-   descript - Descriptive

-   fault - Fault information

-   frontmatter - Front matter

-   ipd - Illustrated parts data

-   learning - Technical training information

-   prdcrossreftable - Product cross-reference table

-   proced - Procedural

-   process - Process

-N  
Omit issue/inwork numbers from filename.

-b &lt;BREX&gt;  
BREX data module code.

-v  
Print the file name of the newly created data module.

-f  
Overwrite existing file.

-s &lt;schema&gt;  
The schema URL.

-S &lt;BREX&gt;  
Determine the tech name from the SNS rules of a specified BREX data module. This can also be specified in the 'defaults' file with the key 'sns'.

-I &lt;date&gt;  
Issue date of the new data module in the form of YYYY-MM-DD.

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

EXAMPLE
=======

s1kd-newdm -\# S1000DTOOLS-A-00-07-00-00A-040A-D -T descript
