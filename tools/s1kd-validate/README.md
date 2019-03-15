NAME
====

s1kd-validate - Validate S1000D CSDB objects against their schemas

SYNOPSIS
========

    s1kd-validate [-d <dir>] [-X <URI>] [-floqvx] [<object>...]

DESCRIPTION
===========

The *s1kd-validate* tool validates S1000D CSDB objects, checking whether
they are valid XML files and if they are valid against their own S1000D
schemas.

OPTIONS
=======

-d &lt;dir&gt;  
Search for schemas in &lt;dir&gt;. Normally, the URI of the schema is
used to fetch it locally or over a network, but this option will force
searching to be performed only in the specified directory.

This can also be accomplished through the use of XML catalogs.

-f  
List invalid files.

-l  
Treat input as a list of object names to validate, rather than an object
itself.

-o  
Output valid CSDB objects to stdout.

-v -q  
Set the verbosity of the output, verbose or quiet. Verbose will
explictly indicate success, rather than simply not displaying any
errors. Quiet will not output anything.

-X &lt;URI&gt;  
Exclude an XML namespace from the validation. Elements in the namespace
specified by &lt;URI&gt; are ignored.

-x  
Do XInclude processing before validation.

--version  
Show version information.

&lt;object&gt;...  
Any number of CSDB objects to validate. If none are specified, input is
read from stdin.

Multi-spec directory with -d option
-----------------------------------

The -d option can point either to a directory containing the XSD schema
files for a single S1000D spec (i.e. the last part of the schema URI),
or to a directory containing schemas for multiple specs. The latter must
follow a particular format for the tool to locate the appropriate
schemas for a given spec:

    schemas/    <-- The directory passed to -d
        S1000D_4-1/
            xml_schema_flat/
                [4.1 XSD files...]
        S1000D_4-2/
            xml_schema_flat/
                [4.2 XSD files...]

XML catalogs vs. -d option
--------------------------

XML catalogs provide a more standard method of redirecting public,
network-based resources to local copies. As part of using libxml2, there
are several locations and environment variables from which this tool
will load catalogs.

Below is an example of a catalog file which maps the S1000D schemas to a
local directory:

    <catalog xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog">
    <rewriteURI uriStartString="http://www.s1000d.org/"
    rewritePrefix="/usr/share/s1kd/schemas/"/>
    </catalog>

This can be placed in a catalog file automatically loaded by libxml2
(e.g., `/etc/xml/catalog`) or saved to a file which is then specified in
an environment variable used by libxml2 (e.g., `XML_CATALOG_FILES`) to
remove the need to use the -d option.

EXAMPLE
=======

    $ s1kd-validate DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
