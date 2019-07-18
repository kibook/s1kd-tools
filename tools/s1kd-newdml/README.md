NAME
====

s1kd-newdml - Create a new S1000D data management list

SYNOPSIS
========

    s1kd-newdml [options] [<object>...]

DESCRIPTION
===========

The *s1kd-newdml* tool creates a new S1000D data management list with
the code and other metadata specified.

OPTIONS
=======

-\#, --code &lt;code&gt;  
The data management list code of the new DML.

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
Save the new DML to &lt;path&gt;. If &lt;path&gt; is an existing
directory, the DML will be created in it instead of the current
directory. Otherwise, the DML will be saved as the filename &lt;path&gt;
instead of being automatically named.

-%, --templates &lt;dir&gt;  
Use the XML template in the specified directory instead of the built-in
template. The template must be named `dml.xml` inside &lt;dir&gt; and
must conform to the default S1000D issue (4.2).

-\~, --dump-templates &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-b, --brex &lt;BREX&gt;  
BREX data module code.

-c, --security &lt;sec&gt;  
The security classification of the new DML.

-d, --defaults &lt;file&gt;  
Specify the `.defaults` file name.

-f, --overwrite  
Overwrite existing file.

-h, -?, --help  
Show usage message.

-I, --date &lt;date&gt;  
The issue date of the new DML in the form of YYYY-MM-DD.

-i, --info-code &lt;info code&gt;  
When creating a DMRL from SNS rules (-S), use the specified info code
for each entry. Specify this option multiple times to create multiple
data modules for each part of the SNS. &lt;info code&gt; can specify:

-   the base info code (e.g., 520)

-   the info code variant (e.g., 520B)

-   the item location code (e.g., 520B-C)

-l, --list  
Treat input (stdin or arguments) as lists of CSDB objects to add to the
new list.

-m, --remarks &lt;remarks&gt;  
Set the remarks for the new data management list.

-N, --omit-issue  
Omit the issue/inwork numbers from filename.

-n, --issno &lt;issue&gt;  
The issue number of the new DML.

-p, --prompt  
Prompts the user for any values left unspecified.

-q, --quiet  
Do not report an error when the file already exists.

-R, --rpccode &lt;NCAGE&gt;  
Specifies a default responsible partner company enterprise code for
entries which do not carry this in their ID STATUS section (ICN, COM,
DML).

-r, --rpcname &lt;name&gt;  
Specifies a default responsible partner company enterprise name for
entries which do not carry this in their IDSTATUS section (ICN, COM,
DML).

-S, --sns &lt;SNS&gt;  
Create a DMRL using the specified SNS rules.

-v, --verbose  
Print the file name of the newly created DML.

-w, --inwork &lt;inwork&gt;  
The inwork number of the new DML.

-z, --issue-type &lt;type&gt;  
The issue type of the new DML.

--version  
Show version information.

&lt;object&gt;...  
Any number of CSDB object file names to automatically add to the list.

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

    $ s1kd-newdml -# EX-12345-C-2018-00001
