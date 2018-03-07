NAME
====

s1kd-metadata - View and edit S1000D data module metadata

SYNOPSIS
========

    s1kd-metadata [-c <file>] [-tf]
                  [-n <name> [-v <value>]]... [<module>]

DESCRIPTION
===========

The *s1kd-metadata* tool provides a simple way to fetch and change metadata on S1000D data modules.

OPTIONS
=======

-c &lt;file&gt;  
Use &lt;file&gt; to edit metadata files. &lt;file&gt; consists of lines starting with a metadata name, followed by whitespace, followed by the new value for the metadata (the program uses this same format when outputting all metadata if no &lt;name&gt; is specified).

-f  
When editing metadata, overwrite the module. The default is to output the modified module to stdout.

-n &lt;name&gt;  
The name of the piece of metadata to fetch. This option can be specified multiple times to fetch multiple pieces of metadata. If -n is not specified, all available metadata names are printed with their values. This output can be sent to a text file, edited, and then specified with the -c option as a means of editing metadata in any text editor.

-t  
Do not format columns in output.

-v &lt;value&gt;  
The new value for the last piece of metadata specified by -n. Each -n can be followed by a -v to edit multiple pieces of metadata.

&lt;module&gt;  
The module to show/edit metadata on. The default is to read from stdin.

EXAMPLE
=======

    $ ls
    DMC-S1000DTOOLS-A-00-09-00-00A-040A-D_EN-CA.XML

    $ s1kd-metadata DMC-S1000DTOOLS-A-00-09-00-00A-040A-D_EN-CA.XML
    issueDate                      2017-08-14
    techName                       s1kd-metadata(1) | s1kd-tools
    responsiblePartnerCompany      khzae.net
    originator                     khzae.net
    securityClassification         01
    schema                         http://www.s1000d.org/S1000D_4-2/xml_
    schema_flat/descript.xsd
    type                           dmodule
    applic                         All
    brex                           S1000D-F-04-10-0301-00A-022A-D
    issueType                      new
    language                       en-CA
    issueInfo                      001-00
    dmCode                         S1000DTOOLS-A-00-09-00-00A-040A-D

    $ s1kd-metadata DMC-S1000DTOOLS-A-00-09-00-00A-040A-D_EN-CA.XML \
      -n techName -v 'New title'
    $ s1kd-metadata DMC-S1000DTOOLS-A-00-09-00-00A-040A-D_EN-CA.XML \
      -n techName
    New title
