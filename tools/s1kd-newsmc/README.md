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

-\#, --code &lt;SMC&gt;  
The SCORM content package code of the new SCORM content package.

-$, --issue &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   5.0 (default)

-   4.2

-   4.1

-@, --out &lt;path&gt;  
Save the new SCORM content package to &lt;path&gt;. If &lt;path&gt; is
an existing directory, the SCORM content package will be created in it
instead of the current directory. Otherwise, the SCORM content package
will be saved as the filename &lt;path&gt; instead of being
automatically named.

-%, --templates &lt;dir&gt;  
Use the XML template in &lt;dir&gt; instead of the built-in template.
The template must be named `scormcontentpackage.xml` in &lt;dir&gt; and
must conform to the default S1000D issue (5.0).

-\~, --dump-templates &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-a, --act &lt;ACT&gt;  
ACT data module code.

-b, --brex &lt;BREX&gt;  
BREX data module code.

-C, --country &lt;country&gt;  
The country ISO code of the new SCORM content package.

-c, --security &lt;sec&gt;  
The security classification of the new SCORM content package.

-D, --include-date  
Include issue date in referenced data modules.

-d, --defaults &lt;file&gt;  
Specify the `.defaults` file name.

-f, --overwrite  
Overwrite existing file.

-h, -?, --help  
Show help/usage message.

-I, --date &lt;date&gt;  
The issue date of the new SCORM content package in the form of
YYYY-MM-DD.

-i, --include-issue  
Include issue information in referenced data modules.

-k, --skill &lt;skill&gt;  
The skill level code of the new SCORM content package.

-L, --language &lt;language&gt;  
The language ISO code of the new SCORM content package.

-l, --include-lang  
Include language information in referenced data modules.

-m, --remarks &lt;remarks&gt;  
Set remarks for the new SCORM content package.

-n, --issno &lt;issue&gt;  
The issue number of the new SCORM content package.

-p, --prompt  
Prompt the user for any values left unspecified.

-q, --quiet  
Do not report an error when the file already exists.

-R, --rpccode &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r, --rpcname &lt;RPC&gt;  
The responsible partner company enterprise name of the new SCORM content
package.

-T, --include-title  
Include titles in referenced data modules.

-t, --title &lt;title&gt;  
The title of the new SCORM content package.

-v, --verbose  
Print the file name of the newly created SCORM content package.

-w, --inwork &lt;inwork&gt;  
The inwork number of the new SCORM content package.

-z, --issue-type &lt;type&gt;  
The issue type of the new SCORM content package.

--version  
Show version information.

&lt;DM&gt;...  
Any number of data modules to automatically reference in the new SCORM
content package's content.

In addition, the following options allow configuration of the XML
parser:

--dtdload  
Load the external DTD.

--huge  
Remove any internal arbitrary parser limits.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--parser-errors  
Emit errors from parser.

--parser-warnings  
Emit warnings from parser.

--xinclude  
Do XInclude processing.

--xml-catalog &lt;file&gt;  
Use an XML catalog when resolving entities. Multiple catalogs may be
loaded by specifying this option multiple times.

`.defaults` file
----------------

Refer to s1kd-newdm(1) for information on the `.defaults` file which is
used by all the s1kd-new\* commands.

EXAMPLE
=======

    $ s1kd-newsmc -# EX-12345-00001-00
