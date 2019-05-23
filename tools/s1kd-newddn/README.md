NAME
====

s1kd-newddn - Create an S1000D Data Dispatch Note (DDN).

SYNOPSIS
========

    s1kd-newddn [options] <files>...

DESCRIPTION
===========

The *s1kd-newddn* tool creates a new S1000D data dispatch note with the
code, metadata, and list of files specified.

OPTIONS
=======

-\#, --code &lt;code&gt;  
The code of the new data dispatch note, in the form of
MODELIDENTCODE-SENDER-RECEIVER-YEAR-SEQUENCE.

-$, --issue &lt;issue&gt;  
Specifiy which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-   2.2

-   2.1

-   2.0

-@, --out &lt;path&gt;  
Save the new DDN to &lt;path&gt;. If &lt;path&gt; is an existing
directory, the DDN will be created in it instead of the current
directory. Otherwise, the DDN will be saved as the filename &lt;path&gt;
instead of being automatically named.

-%, --templates &lt;dir&gt;  
Use the XML template in the specified directory instead of the built-in
template. The template must be named `ddn.xml` inside &lt;dir&gt; and
must conform to the default S1000D issue (4.2).

-\~, --dump-templates &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-a, --authorization &lt;auth&gt;  
Specify the authorization.

-b, --brex &lt;BREX&gt;  
BREX data module code.

-d, --defaults &lt;file&gt;  
Specify the `.defaults` file name.

-f, --overwrite  
Overwrite existing file.

-h, -?, --help  
Show help/usage message.

-I, --date &lt;date&gt;  
The issue date of the new DDN in the form of YYYY-MM-DD.

-m, --remarks &lt;remarks&gt;  
Set the remarks for the new data dispatch note.

-N, --receiver-country &lt;country&gt;  
The receiver's country.

-n, --sender-country &lt;country&gt;  
The sender's country.

-o, --sender &lt;name&gt;  
The enterprise name of the sender.

-p, --prompt  
Prompt the user for values left unspecified.

-q, --quiet  
Do not report an error when the file already exists.

-r, --receiver &lt;name&gt;  
The enterprise name of the receiver.

-T, --receiver-city &lt;city&gt;  
The receiver's city.

-t, --sender-city &lt;city&gt;  
The sender's city.

-v, --verbose  
Print the file name of the newly created DDN.

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

`.defaults` file
----------------

Refer to s1kd-newdm(1) for information on the `.defaults` file which is
used by all the s1kd-new\* commands.

EXAMPLE
=======

    $ s1kd-newddn -# EX-12345-54321-2018-00001
