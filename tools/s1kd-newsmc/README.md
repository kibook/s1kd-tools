NAME
====

s1kd-newsmc - Create new S1000D SCORM content package

SYNOPSIS
========

    s1kd-newsmc [options] [<DM>...]

DESCRIPTION
===========

The *s1kd-newsmc* tool creates a new S1000D SCORM content package with
the SCORM content package code and other metadata specified.

OPTIONS
=======

-\# &lt;SMC&gt;  
The SCORM content package code of the new SCORM content package.

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-@ &lt;path&gt;  
Save the new SCORM content package to &lt;path&gt;. If &lt;path&gt; is
an existing directory, the SCORM content package will be created in it
instead of the current directory. Otherwise, the SCORM content package
will be saved as the filename &lt;path&gt; instead of being
automatically named.

-% &lt;dir&gt;  
Use the XML template in &lt;dir&gt; instead of the built-in template.
The template must be named `scormcontentpackage.xml` in &lt;dir&gt; and
must conform to the default S1000D issue (4.2).

-\~ &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-b &lt;BREX&gt;  
BREX data module code.

-C &lt;country&gt;  
The country ISO code of the new SCORM content package.

-c &lt;sec&gt;  
The security classification of the new SCORM content package.

-D  
Include issue date in referenced data modules.

-d &lt;defaults&gt;  
Specify the `.defaults` file name.

-f  
Overwrite existing file.

-I &lt;date&gt;  
The issue date of the new SCORM content package in the form of
YYYY-MM-DD.

-i  
Include issue information in referenced data modules.

-k &lt;skill&gt;  
The skill level code of the new SCORM content package.

-L &lt;language&gt;  
The language ISO code of the new SCORM content package.

-l  
Include language information in referenced data modules.

-m &lt;remarks&gt;  
Set remarks for the new SCORM content package.

-n &lt;issue&gt;  
The issue number of the new SCORM content package.

-p  
Prompt the user for any values left unspecified.

-q  
Do not report an error when the file already exists.

-R &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r &lt;RPC&gt;  
The responsible partner company enterprise name of the new SCORM content
package.

-T  
Include titles in referenced data modules.

-t &lt;title&gt;  
The title of the new SCORM content package.

-v  
Print the file name of the newly created SCORM content package.

-w &lt;inwork&gt;  
The inwork number of the new SCORM content package.

--version  
Show version information.

&lt;DM&gt;...  
Any number of data modules to automatically reference in the new SCORM
content package's content.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

`.defaults` file
----------------

Refer to s1kd-newdm(1) for information on the `.defaults` file which is
used by all the s1kd-new\* commands.

EXAMPLE
=======

    $ s1kd-newsmc -# EX-12345-00001-00
