NAME
====

s1kd-newdml - Create a new S1000D data management list

SYNOPSIS
========

s1kd-newdml \[options\] &lt;datamodules&gt;

DESCRIPTION
===========

The *s1kd-newdml* tool creates a new S1000D data management list with the code and other metadata specified.

OPTIONS
=======

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-p  
Prompts the user for any values left unspecified.

-\# &lt;code&gt;  
The data management list code of the new DML.

-n &lt;issue&gt;  
The issue number of the new data module.

-w &lt;inwork&gt;  
The inwork number of the new data module.

-c &lt;sec&gt;  
The security classification of the new data module.

-N  
Omit the issue/inwork numbers from filename.

-b &lt;BREX&gt;  
BREX data module code.

-I &lt;date&gt;  
The issue date of the new DML in the form of YYYY-MM-DD.

-v  
Print the file name of the newly created DML.

-f  
Overwrite existing file.

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-@ &lt;filename&gt;  
Save new DML to &lt;filename&gt; instead of an automatically named file in the current directory.

-i  
Include the issue info in the entries of the specified data modules.

-l  
Include the language in the entries of the specified data modules.

-t  
Include the title in the entries of the specified data modules.

-D  
Include the issue date in the entries of the specified data modules.

&lt;datamodules&gt;  
Any number of data module file names to automatically add to the list.

-h -?  
Show usage message.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
