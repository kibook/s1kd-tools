NAME
====

s1kd-newpm - Create new S1000D publication module.

SYNOPSIS
========

    s1kd-newpm [options] [<DM>...]

DESCRIPTION
===========

The *s1kd-newpm* tool creates a new S1000D publication module with the
publication module code and other metadata specified.

OPTIONS
=======

-\#, --code &lt;PMC&gt;  
The publication module code of the new publication module.

-$, --issue &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-   2.2

-   2.1

-   2.0

-@, --out &lt;path&gt;  
Save the new publication module to &lt;path&gt;. If &lt;path&gt; is an
existing directory, the publication module will be created in it instead
of the current directory. Otherwise, the publication module will be
saved as the filename &lt;path&gt; instead of being automatically named.

-%, --templates &lt;dir&gt;  
Use the XML template in &lt;dir&gt; instead of the built-in template.
The template must be named `pm.xml` in &lt;dir&gt; and must conform to
the default S1000D issue (4.2).

-\~, --dump-templates &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-a, --act &lt;ACT&gt;  
ACT data module code.

-b, --brex &lt;BREX&gt;  
BREX data module code.

-C, --country &lt;country&gt;  
The country ISO code of the new publication module.

-c, --security &lt;sec&gt;  
The security classification of the new publication module.

-D, --include-date  
Include issue date in referenced data modules.

-d, --defaults &lt;file&gt;  
Specify the `.defaults` file name.

-f, --overwrite  
Overwrite existing file.

-h, -?, --help  
Show help/usage message.

-I, --date &lt;date&gt;  
The issue date of the new publication module in the form of YYYY-MM-DD.

-i, --include-issue  
Include issue information in referenced data modules.

-L, --language &lt;language&gt;  
The language ISO code of the new publication module.

-l, --include-lang  
Include language information in referenced data modules.

-m, --remarks &lt;remarks&gt;  
Set remarks for the new publication module.

-n, --issno &lt;issue&gt;  
The issue number of the new publication module.

-p, --prompt  
Prompt the user for any values left unspecified.

-q, --quiet  
Do not report an error when the file already exists.

-R, --rpccode &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r, --rpcname &lt;RPC&gt;  
The responsible partner company enterprise name of the new publication
module.

-s, --short-title &lt;title&gt;  
The short title of the new publication module.

-T, --include-title  
Include titles in referenced data modules.

-t, --title &lt;title&gt;  
The title of the new publication module.

-v, --verbose  
Print the file name of the newly created publication module.

-w, --inwork &lt;inwork&gt;  
The inwork number of the new publication module.

-z, --issue-type &lt;type&gt;  
The issue type of the new publication module.

--version  
Show version information.

&lt;DM&gt;...  
Any number of data modules to automatically reference in the new
publication module's content.

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

`.defaults` file
----------------

Refer to s1kd-newdm(1) for information on the `.defaults` file which is
used by all the s1kd-new\* commands.

EXAMPLE
=======

    $ s1kd-newpm -# EX-12345-00001-00
