NAME
====

s1kd-newddn - Create an S1000D DDN (Data Dispatch Note).

SYNOPSIS
========

    s1kd-newddn [options] <files>...

DESCRIPTION
===========

The *s1kd-newddn* tool creates a new S1000D data dispatch note with the code, metadata, and list of files specified.

OPTIONS
=======

-\# &lt;code&gt;  
The code of the new data dispatch note, in the form of MODELIDENTCODE-SENDER-RECEIVER-YEAR-SEQUENCE.

-$ &lt;issue&gt;  
Specifiy which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-@ &lt;filename&gt;  
Save the new DDN as &lt;filename&gt; instead of an automatically named file in the current directory.

-% &lt;dir&gt;  
Use the XML template in the specified directory instead of the built-in template. The template must be named `ddn.xml` inside &lt;dir&gt; and must conform to the default S1000D issue (4.2).

-a &lt;auth&gt;  
Specify the authorization.

-b &lt;BREX&gt;  
BREX data module code.

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-f  
Overwrite existing file.

-h -?  
Show help/usage message.

-I &lt;date&gt;  
The issue date of the new DDN in the form of YYYY-MM-DD.

-N &lt;country&gt;  
The receiver's country.

-n &lt;country&gt;  
The sender's country.

-o &lt;sender&gt;  
The enterprise name of the sender.

-p &lt;showprompts&gt;  
Prompt the user for values left unspecified.

-r &lt;receiver&gt;  
The enterprise name of the receiver.

-T &lt;city&gt;  
The receiver's city.

-t &lt;city&gt;  
The sender's city.

-v  
Print the file name of the newly created DDN.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
