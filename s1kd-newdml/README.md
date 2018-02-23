NAME
====

s1kd-newdml - Create a new S1000D data management list

SYNOPSIS
========

    s1kd-newdml [options] [<datamodules>]

DESCRIPTION
===========

The *s1kd-newdml* tool creates a new S1000D data management list with the code and other metadata specified.

OPTIONS
=======

-\# &lt;code&gt;  
The data management list code of the new DML.

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-@ &lt;filename&gt;  
Save new DML to &lt;filename&gt; instead of an automatically named file in the current directory.

-% &lt;dir&gt;  
Use the XML template in the specified directory instead of the built-in template. The template must be named `dml.xml` inside &lt;dir&gt; and must conform to the default S1000D issue (4.2).

-b &lt;BREX&gt;  
BREX data module code.

-c &lt;sec&gt;  
The security classification of the new data module.

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-f  
Overwrite existing file.

-h -?  
Show usage message.

-I &lt;date&gt;  
The issue date of the new DML in the form of YYYY-MM-DD.

-N  
Omit the issue/inwork numbers from filename.

-n &lt;issue&gt;  
The issue number of the new data module.

-p  
Prompts the user for any values left unspecified.

-R &lt;NCAGE&gt;  
Specifies a default responsible partner company enterprise code for entries which do not carry this in their ID STATUS section (ICN, COM, DML).

-r &lt;name&gt;  
Specifies a default responsible partner company enterprise name for entries which do not carry this in their IDSTATUS section (ICN, COM, DML).

-v  
Print the file name of the newly created DML.

-w &lt;inwork&gt;  
The inwork number of the new data module.

&lt;datamodules&gt;  
Any number of data module file names to automatically add to the list.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
