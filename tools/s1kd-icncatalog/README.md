NAME
====

s1kd-icncatalog - Manage the catalog used to resolve ICNs

SYNOPSIS
========

    s1kd-icncatalog [options] [<object>...]

DESCRIPTION
===========

The *s1kd-icncatalog* tool is used to manage a catalog of ICNs for a
project, and to resolve ICNs using this catalog. Resolving an ICN means
placing the actual filename of the ICN in to the SYSTEM ID of the ENTITY
declaration within CSDB objects.

OPTIONS
=======

-a, --add &lt;ICN&gt;  
Add an ICN to the catalog. Follow with the -u and -n options to specify
the URI and notation to use for this ICN. The -m option specifies a
media group to add the ICN to.

-c, --catalog &lt;catalog&gt;  
Specify the catalog file to manage or resolve against. By default, the
file `.icncatalog` in the current directory is used. If the current
directory does not contain this file, the parent directories will be
searched.

-d, --del &lt;ICN&gt;  
Delete an ICN from the catalog. The -m option specifies a media group to
delete the ICN from.

-f, --overwrite  
Overwrite the input CSDB objects when resolving ICNs, or overwrite the
catalog file when modifying it. Otherwise, output is written to stdout.

-h, -?, --help  
Show help/usage message.

-l, --list  
Treat input (stdin or arguments) as lists of filenames of CSDB objects,
rather than CSDB objects themselves.

-m, --media &lt;media&gt;  
Resolve ICNs for this intended output media. The catalog may contain
alternative formats for the same ICN to be used for different output
media.

-n, --ndata &lt;notation&gt;  
Specify the notation to reference when adding an ICN with the -a option.

-t, --new  
Create a new empty catalog.

-u, --uri &lt;URI&gt;  
Specify the URI when adding an ICN with the -a option.

-v, --verbose  
Verbose output.

--version  
Show version information.

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

Resolving ICNs to filenames
---------------------------

A CSDB object may reference an ICN as follows:

    <!NOTATION png SYSTEM "png">
    <!ENTITY ICN-12345-00001-001-01 SYSTEM "ICN-12345-00001-001-01.PNG"
    NDATA png>

The SYSTEM ID of this ENTITY indicates that the ICN file will be in the
same directory relative to the CSDB object. However, the ICN files in
this example are located in a separate folder called 'graphics'. Rather
than manually updating every ENTITY declaration in every CSDB object, a
catalog file can be used to map ICNs to actual filenames:

    <icnCatalog>
    <icn infoEntityIdent="ICN-12345-00001-001-01"
    uri="graphics/ICN-12345-00001-001-01.PNG"/>
    </icnCatalog>

Then, using this tool, the ICN can be resolved against the catalog:

    $ s1kd-icncatalog -c <catalog> <object>

Producing the following output:

    <!NOTATION png SYSTEM "png">
    <!ENTITY ICN-12345-00001-001-01 SYSTEM
    "graphics/ICN-12345-00001-001-01.PNG" NDATA png>

Alternative ICN formats
-----------------------

A catalog can also be used to provide alternative file formats for an
ICN depending on the intended output media. For example:

    <icnCatalog>
    <notation name="jpg" systemId="jpg"/>
    <notation name="svg" systemId="svg"/>
    <media name="pdf">
    <icn infoEntityIdent="ICN-12345-00001-001-01"
    uri="ICN-12345-00001-001-01.JPG" notation="jpg"/>
    </media>
    <media name="web">
    <icn infoEntityIdent="ICN-12345-00001-001-01"
    uri="ICN-12345-00001-001-01.SVG" notation="svg"/>
    </media>
    </icnCatalog>

The -m option allows for specifying which type of media to resolve for:

    <!NOTATION png SYSTEM "png">
    <!ENTITY ICN-12345-00001-001-01 SYSTEM "ICN-12345-00001-001-01.PNG"
    NDATA png>

    $ s1kd-icncatalog -c <catalog> -m pdf <object>

    <!NOTATION png SYSTEM "png">
    <!NOTATION jpg SYSTEM "jpg">
    <!ENTITY ICN-12345-00001-001-01 SYSTEM "ICN-12345-00001-001-01.JPG"
    NDATA jpg>

    $ s1kd-icncatalog -c <catalog> -m web <object>

    <!NOTATION png SYSTEM "png">
    <!NOTATION svg SYSTEM "svg">
    <!ENTITY ICN-12345-00001-001-01 SYSTEM "ICN-12345-00001-001-01.SVG"
    NDATA svg>

Reconstructing ICN entity declarations
--------------------------------------

Some processing, such as XSL transformations, may remove the DTD and
external entity declarations as part of parsing an XML CSDB object. A
catalog can be used to restore the necessary external entity
declarations afterwards. For example:

    $ xsltproc ex.xsl <object>

The resulting XML will not include a DTD or the external entity
declarations for the ICNs referenced in the object, so it will not be
valid according to the S1000D schema:

    $ xsltproc ex.xsl <object> | s1kd-validate
    -:49:element graphic: Schemas validity error: Element 'graphic',
    attribute 'infoEntityIdent': 'ICN-12345-00001-001-01' is not a valid
    value of the atomic type 'xs:ENTITY'.

Passing the result to this tool, with a catalog containing all the ICNs
used by the project:

    $ xsltproc ex.xsl <object> | s1kd-icncatalog -c <catalog>

will reconstruct the required external entity declarations in the DTD.

> **Note**
>
> The s1kd-tools will copy the DTD and external entity declarations
> automatically when performing transformations (such as with the
> s1kd-transform tool), so this is only necessary when using more
> generic XML tools.

CATALOG SCHEMA
==============

The following describes the schema of an ICN catalog file.

Catalog
-------

*Markup element:* `<icnCatalog>`

*Attributes:*

-   None

*Child elements:*

-   `<notation>`

-   `<media>`

-   `<icn>`

Notation
--------

The element `<notation>` represents a NOTATION declaration.

*Markup element:* `<notation>`

*Attributes:*

-   `name`, the NDATA name.

-   `publicId`, the optional PUBLIC ID of the notation.

-   `systemId`, the optional SYSTEM ID of the notation.

*Child elements:*

-   None

Media
-----

The element `<media>` groups a set of alternative ICN formats for a
particular output media type.

*Markup element:* `<media>`

*Attributes:*

-   `name`, the identifier of the output media.

*Child elements:*

-   `<icn>`

ICN
---

The element `<icn>` maps an ICN to a filename and optionally a notation.
When this element occurs as a child of a `<media>` element, it will be
used when that output media is specified with the -m option. When it
occurs as a child of `<icnCatalog>`, it will be used if no media is
specified.

*Markup element:* `<icn>`

*Attributes:*

-   `infoEntityIdent`, the ICN

-   `uri`, the filename the ICN will resolve to

-   `notation`, a reference to a previously declared `<notation>`
    element.

*Child elements:*

-   None

Example ICN catalog
-------------------

    <icnCatalog>
    <notation name="jpg" systemId="jpg"/>
    <notation name="png" systemId="png"/>
    <notation name="svg" systemId="svg"/>
    <media name="pdf">
    <icn infoEntityIdent="ICN-12345-00001-001-01"
    uri="ICN-12345-00001-001-01.JPG" notation="jpg"/>
    </media>
    <media name="web">
    <icn infoEntityIdent="ICN-12345-00001-001-01"
    uri="ICN-12345-00001-001-01.SVG" notation="svg"/>
    </media>
    <icn infoEntityIdent="ICN-12345-00001-001-01"
    uri="ICN-12345-00001-001-01.PNG" notation="png"/>
    </icnCatalog>
