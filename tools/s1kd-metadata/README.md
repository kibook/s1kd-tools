NAME
====

s1kd-metadata - View and edit S1000D CSDB object metadata

SYNOPSIS
========

    s1kd-metadata [options] [<object>...]

DESCRIPTION
===========

The *s1kd-metadata* tool provides a simple way to fetch and change
metadata on S1000D CSDB objects.

OPTIONS
=======

-0  
Print a null-delimited list of values of the pieces of metadata
specified with -n, or all available metadata if -n is not specified.

-c &lt;file&gt;  
Use &lt;file&gt; to edit metadata files. &lt;file&gt; consists of lines
starting with a metadata name, followed by whitespace, followed by the
new value for the metadata (the program uses this same format when
outputting all metadata if no &lt;name&gt; is specified).

-e  
When showing all metadata, only list editable items. This is useful when
creating a file for use with the -c option.

-F &lt;fmt&gt;  
Print a formatted line for each CSDB object. Metadata names surrounded
with % (e.g. %issueDate%) will be substituted by the value read from the
object.

-f  
When editing metadata, overwrite the object. The default is to output
the modified object to stdout.

-H  
Lists all available metadata with a short description of each. Specify
specific metadata to describe with the -n option.

-l  
Treat input as a list of object filenames to read or edit metadata on,
rather than an object itself.

-n &lt;name&gt;  
The name of the piece of metadata to fetch. This option can be specified
multiple times to fetch multiple pieces of metadata. If -n is not
specified, all available metadata names are printed with their values.
This output can be sent to a text file, edited, and then specified with
the -c option as a means of editing metadata in any text editor.

-q  
Quiet mode. Non-fatal errors such as a missing piece of optional
metadata in an object will not be printed to stderr.

-T  
Do not format columns in output.

-t  
Print a tab-delimited list of values of the pieces of metadata specified
with -n, or all available metadata if -n is not specified.

-v &lt;value&gt;  
When following a -n option, this specifies the new value for that piece
of metadata.

When following a -w or -W option, this specifies the value to compare
that piece of metadata to.

Each -n, -w, or -W can be followed by -v to edit or define conditions on
multiple pieces of metadata.

-W &lt;name&gt;  
Show or edit metadata only on objects where the value of &lt;name&gt; is
not equal to the value specified in the following -v option. If no -v
option follows, this will show objects which do not have metadata
&lt;name&gt; of any value.

-w &lt;name&gt;  
Show or edit metadata only on objects where the value of &lt;name&gt; is
equal to the value specified in the following -v option. If no -v option
follows, this will show objects which have metadata &lt;name&gt; with
any value.

--version  
Show version information.

&lt;object&gt;...  
The object(s) to show/edit metadata on. The default is to read from
stdin.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

EXAMPLE
=======

    $ ls
    DMC-S1KDTOOLS-A-00-09-00-00A-040A-D_EN-CA.XML
    DMC-S1KDTOOLS-A-00-0Q-00-00A-040A-D_EN-CA.XML

    $ DMOD=DMC-S1KDTOOLS-A-00-09-00-00A-040A-D_EN-CA.XML
    $ s1kd-metadata $DMOD
    issueDate                      2017-08-14
    techName                       s1kd-metadata(1) | s1kd-tools
    responsiblePartnerCompany      khzae.net
    originator                     khzae.net
    securityClassification         01
    schema                         descript
    schemaUrl                      http://www.s1000d.org/S1000D_4-2/xml_
    schema_flat/descript.xsd
    type                           dmodule
    applic                         All
    brex                           S1000D-F-04-10-0301-00A-022A-D
    issueType                      new
    languageIsoCode                en
    countryIsoCode                 CA
    issueNumber                    001
    inWork                         00
    dmCode                         S1KDTOOLS-A-00-09-00-00A-040A-D

    $ s1kd-metadata -n techName -v "New title" $DMOD
    $ s1kd-metadata -n techName $DMOD
    New title

    $ s1kd-metadata -n techName DMC-*.XML
    New title
    s1kd-aspp(1) | s1kd-tools

    $ s1kd-metadata -F "%techName% (%issueDate%) %issueType%" DMC-*.XML
    New title (2017-08-14) new
    s1kd-aspp(1) | s1kd-tools (2018-03-28) changed

    $ s1kd-metadata -F "%techName%" -w subSubSystemCode -v Q DMC-*.XML
    s1kd-aspp(1) | s1kd-tools
