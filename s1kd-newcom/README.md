NAME
====

s1kd-newcom - Create a new S1000D comment.

SYNOPSIS
========

s1kd-newcom \[options\]

DESCRIPTION
===========

The *s1kd-newcom* tool creates a new S1000D comment with the code and metadata specified.

OPTIONS
=======

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-p  
Prompt the user for values left unspecified.

-\# &lt;code&gt;  
The code of the comment, in the form of MODELIDENTCODE-SENDERIDENT-YEAR-SEQ-TYPE.

-L &lt;lang&gt;  
The language ISO code of the new comment.

-C &lt;country&gt;  
The country ISO code of the new comment.

-c &lt;sec&gt;  
The security classification of the new comment.

-o &lt;orig&gt;  
The enterprise name of the originator of the comment.

-t &lt;title&gt;  
The title of the new comment.

-r &lt;type&gt;  
The response type of the new comment.

-b &lt;BREX&gt;  
BREX data module code.

-I &lt;date&gt;  
The issue date of the new comment in the form of YYYY-MM-DD.

-v  
Print the file name of the newly created comment.

-f  
Overwrite existing file.

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.
