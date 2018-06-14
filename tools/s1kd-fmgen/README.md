NAME
====

s1kd-fmgen - Generate front matter data module contents

SYNOPSIS
========

    s1kd-fmgen [-F <FMTYPES>] [-p <PM>] [-X <XSL>] [-,.fhx?]
               (-t <TYPE>|<DM>...)

DESCRIPTION
===========

The *s1kd-fmgen* tool generates the content section for front matter data modules from either a standard publication module, or the combined format of the s1kd-flatten(1) tool. Some front matter types require the use of the combined format, particularly those that list information not directly found in the publication module, such as the highlights (HIGH) type.

OPTIONS
=======

-,  
Dump the built-in `.fmtypes` XML format.

-.  
Dump the built-in `.fmtypes` simple text format.

-h -?  
Show usage message.

-F &lt;FMTYPES&gt;  
Specify a custom `.fmtypes` file.

-f  
Overwrite the specified front matter data module files after generating their content.

-p &lt;PM&gt;  
Publication module or s1kd-flatten(1) PM format file to generate contents from. If none is specified, the tool will read from stdin.

-t &lt;TYPE&gt;  
Generate content for this type of front matter when no data modules are specified. Supported types are:

-   HIGH - Highlights

-   LOEDM - List of effective data modules

-   TOC - Table of contents

-   TP - Title page

-X &lt;XSL&gt;  
Transform the front matter contents after generating them using the specified XSLT. This can be used, for example, to generate content for a descriptive schema data module instead, to support older issues of the specification, or for types of generated front matter not covered by the frontmatter schema.

-x  
Do XInclude processing.

&lt;DM&gt;...  
Front matter data modules to generate content for.

`.fmtypes` file
---------------

This file specifies a list of info codes to associate with a particular type of front matter. By default, the program will search for a file named `.fmtypes` in the current directory, but any file can be specified using the -F option.

Example of simple text format:

    001    TP
    009    TOC
    00S    LOEDM
    00U    HIGH

Example of XML format

    <fmtypes>
    <fm infoCode="001" type="TP"/>
    <fm infoCode="009" type="TOC"/>
    <fm infoCode="00S" type="LOEDM"/>
    </fmtypes>

EXAMPLE
=======

Generate the content for a title page front matter data module and overwrite the file:

    $ s1kd-flatten PMC-EX-12345-00001-00_001-00_EN-CA.XML |
    > s1kd-fmgen -f DMC-EX-A-00-00-00-00A-001A-D_001-00_EN-CA.XML
