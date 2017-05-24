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

-o &lt;orig&gt;  
The originator enterprise name of the new data module.

-t &lt;tech&gt;  
The tech name of the new data module.

-i &lt;info&gt;  
The info name of the new data module.

-T &lt;schema&gt;  
The type (schema) of the new data module. Supported schemas:

-   appliccrossreftable - Applicability cross-reference table

-   brdoc - Business rule document

-   brex - Business rule exchange

-   comrep - Common information repository

-   descript - Descriptive

-   frontmatter - Front matter

-   proced - Procedural

Prompt (-p) option
------------------

If this option is specified, the program will prompt the user to enter values for metadata which was not specified when calling the program. If a piece of metadata has a default value (from the 'defaults' and 'dmtypes' files), it will be displayed in square brackets \[\] in the prompt, and pressing Enter without typing any value will select this default value.

'defaults' file
---------------

This file sets default values for each piece of metadata. By default, the program will search the current directory for a file named 'defaults', but any file can be specified by using the -d option.

Each line consists of the identifier of a piece of metadata and its default value, separated by whitespace. Example:

    modelIdentCode               S1000DTOOLS
    systemDiffCode               A
    systemCode                   00
    subSystemCode                0
    subSubSystemCode             0
    assyCode                     00
    disassyCode                  00
    disassycodeVariant           A
    infoCode                     040
    infoCodeVariant              A
    itemLocationCode             D
    languageIsoCode              en
    countryIsoCode               CA
    issueNumber                  000
    inWork                       01
    securityClassification       01
    responsiblePartnerCompany    khzai.net
    originator                   khzai.net

'dmtypes' file
--------------

This file sets the default type (schema) for data modules based on their info code. By default, the program will search the current directory for a file named 'dmtypes', but any file can be specified by using the -D option.

Each line consists of an info code and a schema identifier. Example:

    00E    comrep
    00W    appliccrossreftable
    009    frontmatter
    022    brex
    024    brdoc
    040    descript
    520    proced

EXAMPLE
=======

s1kd-newdm -\# S1000DTOOLS-A-00-07-00-00A-040A-D -T descript
