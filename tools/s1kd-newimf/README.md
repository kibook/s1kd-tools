NAME
====

s1kd-newimf - Create a new S1000D ICN metadata file

SYNOPSIS
========

    s1kd-newimf [options] <ICNs>...

DESCRIPTION
===========

The *s1kd-newimf* tool creates a new S1000D ICN metadata file for
specified ICN files.

OPTIONS
=======

-$, --issue &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   5.0 (default)

-   4.2

-@, --out &lt;path&gt;  
Save the new IMF to &lt;path&gt;. If &lt;path&gt; is an existing
directory, the IMF will be created in it instead of the current
directory. Otherwise, the IMF will be saved as the filename &lt;path&gt;
instead of being automatically named.

-%, --templates &lt;dir&gt;  
Use the XML template in &lt;dir&gt; instead of the built-in template.
The template must be named `icnmetadata.xml` inside &lt;dir&gt; and must
conform to the default S1000D issue (5.0).

-\~, --dump-templates &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-b, --brex &lt;BREX&gt;  
BREX data module code.

-c, --security &lt;sec&gt;  
The security classification of the new ICN metadata file.

-d, --defaults &lt;file&gt;  
Specify the `.defaults` file name.

-f, --overwrite  
Overwrite existing file.

-h, -?, --help  
Show help/usage message.

-I, --date &lt;date&gt;  
The issue date of the new ICN metadata file in the form of YYYY-MM-DD.

-m, --remarks &lt;remarks&gt;  
Set the remarks for the new ICN metadata file.

-n, --issno &lt;issue&gt;  
The issue number of the new ICN metadata file.

-O, --origcode &lt;CAGE&gt;  
The CAGE code of the originator.

-o, --origname &lt;orig&gt;  
The originator enterprise name of the new ICN metadata file.

-p, --prompt  
Prompts the user for any values left unspecified.

-q, --quiet  
Do not report an error when the file already exists.

-R, --rpccode &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r, --rpcname &lt;RPC&gt;  
The responsible partner company enterprise name of the new ICN metadata
file.

-t, --title &lt;title&gt;  
The ICN title (if creating multiple ICNs, they will all use this title).

-v, --verbose  
Print the file name of the newly created IMF.

-w, --inwork &lt;inwork&gt;  
The inwork issue of the new ICN metadata file.

--version  
Show version information.

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

Refer to s1kd-newdm(1) for information on the `.defaults` file used by
all the s1kd-new\* tools.

EXAMPLE
=======

    $ s1kd-newimf ICN-EX-00001-001-01.PNG
