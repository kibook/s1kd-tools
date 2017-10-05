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

-v  
Print the file name of the newly created DML.

&lt;datamodules&gt;  
Any number of data module file names to automatically add to the list.

-h -?  
Show usage message.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
