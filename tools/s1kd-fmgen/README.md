# NAME

s1kd-fmgen - Generate front matter data module contents

# SYNOPSIS

    s1kd-fmgen [-D <TYPE>] [-F <FMTYPES>] [-I <date>] [-P <PM>]
               [-p <name>=<val> ...] [-t <TYPE>] [-x <XSL>]
               [-,.flqvh?] [<DM>...]

# DESCRIPTION

The *s1kd-fmgen* tool generates the content section for front matter
data modules from either a standard publication module, or the combined
format of the s1kd-flatten(1) tool. Some front matter types require the
use of the combined format, particularly those that list information not
directly found in the publication module, such as the highlights (HIGH)
type.

# OPTIONS

  - \-,, --dump-fmtypes-xml  
    Dump the built-in `.fmtypes` XML format.

  - \-., --dump-fmtypes  
    Dump the built-in `.fmtypes` simple text format.

  - \-D, --dump-xsl \<TYPE\>  
    Dump the built-in XSLT used to generate the specified type of front
    matter.

  - \-F, --fmtypes \<FMTYPES\>  
    Specify a custom `.fmtypes` file.

  - \-f, --overwrite  
    Overwrite the specified front matter data module files after
    generating their content.

  - \-h, -?, --help  
    Show usage message.

  - \-I, --date \<date\>  
    Set the issue date of the generated front matter data modules. This
    can be a specific date in the form of "YYYY-MM-DD", "-" for the
    current date, or "pm" to use the issue date of the publication
    module.

  - \-l, --list  
    Treat input (stdin or arguments) as lists of front matter data
    modules to generate content for, rather than data modules
    themselves. If reading list from stdin, the -P option must be used
    to specify the publication module.

  - \-P, --pm \<PM\>  
    Publication module or s1kd-flatten(1) PM format file to generate
    contents from. If none is specified, the tool will read from stdin.

  - \-p, --param \<name\>=\<value\>  
    Pass a parameter to the XSLT stylesheets used to generate the front
    matter content. Multiple parameters can be specified by using this
    option multiple times.
    
    The following parameters are automatically supplied to any
    stylesheet, and therefore their names should be considered reserved:
    
      - `"type"` - The front matter type name (e.g., HIGH) that was
        matched in the `.fmtypes` file or specified by the user with the
        -t option.

  - \-q, --quiet  
    Quiet mode. Do not print errors.

  - \-t, --type \<TYPE\>  
    Generate content for this type of front matter. Supported types are:
    
      - HIGH - Highlights
    
      - LOA - List of abbreviations
    
      - LOASD - List of applicable specifications and documentation
    
      - LOEDM - List of effective data modules
    
      - LOI - List of illustrations
    
      - LOS - List of symbols
    
      - LOT - List of terms
    
      - LOTBL - List of tables
    
      - TOC - Table of contents
    
      - TITLE - Title page

  - \-v, --verbose  
    Verbose output. Specify multiple times to increase the verbosity.

  - \-x, --xsl \<XSL\>  
    Use the specified XSLT script to generate the front matter contents
    instead of the built-in XSLT or the user-configured XSLT from the
    `.fmtypes` file.

  - \--version  
    Show version information.

  - \<DM\>...  
    Front matter data modules to generate content for. If no front
    matter type can be determined for a data module, it will be ignored.

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

## `.fmtypes` file

This file specifies a list of info codes to associate with a particular
type of front matter.

Optionally, a path to an XSLT script can be given for each info code,
which will be used to generate the front matter instead of the built-in
XSLT. The path to an XSLT script will be interpreted relative to the
location of the `.fmtypes` file (typically, the top directory of the
CSDB). The -D option can be used to dump the built-in XSLT for a type of
front matter as a starting point for a custom script.

Optionally, in the XML format, the attribute `ignoreDel` may be
specified to control whether deleted data modules and elements are
ignored when generating front matter contents. These are data modules
with an issue type of "`deleted`" and elements with a change type of
"`delete`". A value of "`yes`" means deleted content will not be
included, while "`no`" means it will. If this attribute is not
specified, then a default value will be used based on the type of front
matter. The following types will ignore deleted content by default:

  - LOA

  - LOASD

  - LOI

  - LOS

  - LOTBL

  - TOC

  - TITLE

By default, the program will search for a file named `.fmtypes` in the
current directory and parent directories, but any file can be specified
using the -F option.

Example of simple text format:

    001    TITLE
    005    LOA
    006    LOT
    007    LOS
    009    TOC
    00A    LOA
    00S    LOEDM
    00U    HIGH    fm/high.xsl
    00V    LOASD
    00Z    LOTBL

Example of XML format:

    <fmtypes>
    <fm infoCode="001" type="TITLE"/>
    <fm infoCode="005" type="LOA"/>
    <fm infoCode="006" type="LOT"/>
    <fm infoCode="007" type="LOS"/>
    <fm infoCode="009" type="TOC"/>
    <fm infoCode="00A" type="LOI"/>
    <fm infoCode="00S" type="LOEDM"/>
    <fm infoCode="00U" type="HIGH" xsl="fm/high.xsl"/>
    <fm infoCode="00V" type="LOASD"/>
    <fm infoCode="00Z" type="LOTBL"/>
    </fmtypes>

The info code of each entry in the `.fmtypes` file may also include an
info code variant. This allows different transformations to be used
based on the variant:

    <fmtypes>
    <fm infoCode="00UA" type="HIGH" xsl="fm/high.xsl"/>
    <fm infoCode="00UB" type="HIGH" xsl="fm/high-updates.xsl"/>
    <fm infoCode="00U"  type="HIGH"/>
    </fmtypes>

In the example above, a highlights data module (00U) with info code
variant A will use an XSL transformation that creates a simple
highlights, while a highlights data module with info code variant B will
use an XSL transformation that creates a highlights with update
instructions. All other variants will use the built-in XSLT.

Entries are chosen in the order they are listed in the `.fmtypes` file.
An info code which does not specify a variant matches all possible
variants.

## Optional title page elements

When re-generating the front matter content for a title page data
module, optional elements which cannot be derived from the publication
module (such as the product illustration or bar code) will be copied
from the source data module when updating it.

## Multi-pass transforms

Rather than a literal XSLT file, the path specified for the `xsl`
attribute in the `.fmtypes` file or the -x (--xsl) option may be an
XProc file which contains a pipeline with multiple stylesheets. This
allows for multi-pass transformations.

> **Note**
> 
> Only a small subset of XProc is supported at this time.

Example:

    <p:pipeline
    xmlns:p="http://www.w3.org/ns/xproc"
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    version="1.0">
    <p:xslt name="Pass 1">
    <p:input port="stylesheet">
    <p:document href="pass1.xsl"/>
    </p:input>
    <p:with-param name="update-instr" select="true()"/>
    </p:xslt>
    <p:xslt name="Pass 2">
    <p:input port="stylesheet">
    <p:inline>
    <xsl:transform version="1.0">
    ...
    </xsl:transform>
    </p:inline>
    </p:input>
    </p:xslt>
    </p:pipeline>

# EXIT STATUS

  - 0  
    No errors.

  - 1  
    The date specified with -I is invalid.

  - 2  
    No front matter types were specified.

  - 3  
    An unknown front matter type was specified.

  - 4  
    The resulting front matter content could not be merged in to a data
    module.

  - 5  
    The stylesheet specified for a type of front matter was invalid.

  - 6  
    The transformation of a front matter data module failed.

  - 7  
    The publication module could not be read.

# EXAMPLE

Generate the content for a title page front matter data module and
overwrite the file:

    $ s1kd-flatten PMC-EX-12345-00001-00_001-00_EN-CA.XML |
    > s1kd-fmgen -f DMC-EX-A-00-00-00-00A-001A-D_001-00_EN-CA.XML
