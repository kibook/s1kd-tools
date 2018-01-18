NAME
====

s1kd-newddn - Create an S1000D DDN (Data Dispatch Note).

SYNOPSIS
========

s1kd-newddn \[options\] &lt;files&gt;...

DESCRIPTION
===========

The *s1kd-newddn* tool creates a new S1000D data dispatch note with the code, metadata, and list of files specified.

OPTIONS
=======

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-p &lt;showprompts&gt;  
Prompt the user for values left unspecified.

-\# &lt;code&gt;  
The code of the new data dispatch note, in the form of MODELIDENTCODE-SENDER-RECEIVER-YEAR-SEQUENCE.

-o &lt;sender&gt;  
The enterprise name of the sender.

-r &lt;receiver&gt;  
The enterprise name of the receiver.

-t &lt;city&gt;  
The sender's city.

-T &lt;city&gt;  
The receiver's city.

-n &lt;country&gt;  
The sender's country.

-N &lt;country&gt;  
The receiver's country.

-a &lt;auth&gt;  
Specify the authorization.

-h -?  
Show help/usage message.

-b &lt;BREX&gt;  
BREX data module code.

-I &lt;date&gt;  
The issue date of the new DDN in the form of YYYY-MM-DD.

-v  
Print the file name of the newly created DDN.

-f  
Overwrite existing file.

-$ &lt;issue&gt;  
Specifiy which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-@ &lt;filename&gt;  
Save the new DDN as &lt;filename&gt; instead of an automatically named file in the current directory.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
