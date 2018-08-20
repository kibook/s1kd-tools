NAME
====

s1kd-newimf - Create a new S1000D ICN metadata file

SYNOPSIS
========

    s1kd-newimf [options] <ICNs>...

DESCRIPTION
===========

The *s1kd-newimf* tool creates a new S1000D ICN metadata file for specified ICN files.

OPTIONS
=======

-% &lt;dir&gt;  
Use the XML template in &lt;dir&gt; instead of the built-in template. The template must be named `icnmetadata.xml` inside &lt;dir&gt; and must conform to the default S1000D issue (4.2).

-~ &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-b &lt;BREX&gt;  
BREX data module code.

-c &lt;sec&gt;  
The security classification of the new ICN metadata file.

-d &lt;defaults&gt;  
Specify the `.defaults` file name.

-f  
Overwrite existing file.

-I &lt;date&gt;  
The issue date of the new ICN metadata file in the form of YYYY-MM-DD.

-m &lt;remarks&gt;  
Set the remarks for the new ICN metadata file.

-n &lt;issue&gt;  
The issue number of the new ICN metadata file.

-O &lt;CAGE&gt;  
The CAGE code of the originator.

-o &lt;orig&gt;  
The originator enterprise name of the new ICN metadata file.

-p  
Prompts the user for any values left unspecified.

-q  
Do not report an error when the file already exists.

-R &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-r &lt;RPC&gt;  
The responsible partner company enterprise name of the new ICN metadata file.

-t &lt;title&gt;  
The ICN title (if creating multiple ICNs, they will all use this title).

-v  
Print the file name of the newly created IMF.

-w &lt;inwork&gt;  
The inwork issue of the new ICN metadata file.

--version  
Show version information.

`.defaults` file
----------------

Refer to s1kd-newdm(1) for information on the `.defaults` file used by all the s1kd-new\* tools.

EXAMPLE
=======

    $ s1kd-newimf ICN-EX-00001-001-01.PNG
