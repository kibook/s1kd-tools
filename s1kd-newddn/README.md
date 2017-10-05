NAME
====

s1kd-newddn - Create an S1000D data dispatch note.

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

-v  
Print the file name of the newly created DDN.

-f  
Overwrite existing file.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
