NAME
====

s1kd-newimf - Create a new S1000D ICN metadata file

SYNOPSIS
========

s1kd-newimf \[options\] &lt;ICNs&gt;...

DESCRIPTION
===========

The *s1kd-newimf* tool creates a new S1000D ICN metadata file for specified ICN files.

OPTIONS
=======

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-p  
Prompts the user for any values left unspecified.

-n &lt;issue&gt;  
The issue number of the new ICN metadata file.

-w &lt;inwork&gt;  
The inwork issue of the new ICN metadata file.

-c &lt;sec&gt;  
The security classification of the new ICN metadata file.

-r &lt;RPC&gt;  
The responsible partner company enterprise name of the new ICN metadata file.

-R &lt;CAGE&gt;  
The CAGE code of the responsible partner company.

-o &lt;orig&gt;  
The originator enterprise name of the new ICN metadata file.

-O &lt;CAGE&gt;  
The CAGE code of the originator.

-t &lt;title&gt;  
The ICN title (if creating multiple ICNs, they will all use this title).

-b &lt;BREX&gt;  
BREX data module code.

-I &lt;date&gt;  
The issue date of the new ICN metadata file in the form of YYYY-MM-DD.

-v  
Print the file name of the newly created IMF.

-f  
Overwrite existing file.
