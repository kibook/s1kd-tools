NAME
====

s1kd-metadata - View and edit S1000D data module metadata

SYNOPSIS
========

s1kd-metadata \[-c &lt;file&gt;\] \[-t\] \[&lt;name&gt; \[&lt;value&gt;\]\]

DESCRIPTION
===========

The *s1kd-metadata* tool provides a simple way to fetch and change metadata on S1000D data modules.

OPTIONS
=======

-c &lt;file&gt;  
Use &lt;file&gt; to edit metadata files. &lt;file&gt; consists of lines starting with a metadata name, followed by whitespace, followed by the new value for the metadata (the program uses this same format when outputting all metadata if no &lt;name&gt; is specified).

&lt;name&gt;  
The name of the piece of metadata to fetch. If no name is specified, all available metadata names are printed with their values. This output can be sent to a text file, edited, and then specified with the -c option as a means of editing metadata in any text editor.

&lt;value&gt;  
The new value for the piece of metadata.

Available metadata names
------------------------

-   act

-   applic

-   authorization

-   brex

-   language

-   infoName

-   issueDate

-   issueInfo

-   issueType

-   originator

-   originatorCode

-   responsiblePartnerCompany

-   responsiblePartnerCompanyCode

-   schema

-   securityClassification

-   techName

-   type
