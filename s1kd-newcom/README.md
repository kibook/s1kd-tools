NAME
====

s1kd-newcom - Create a new S1000D comment.

SYNOPSIS
========

    s1kd-newcom [options]

DESCRIPTION
===========

The *s1kd-newcom* tool creates a new S1000D comment with the code and metadata specified.

OPTIONS
=======

-\# &lt;code&gt;  
The code of the comment, in the form of MODELIDENTCODE-SENDERIDENT-YEAR-SEQ-TYPE.

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-   4.0

-   3.0

-   2.3

-   2.2

-   2.1

-   2.0

-@ &lt;filename&gt;  
Save the new comment as &lt;filename&gt; instead of an automatically named file in the current directory.

-% &lt;dir&gt;  
Use the XML template in the specified directory instead of the built-in template. The template must be named `comment.xml` inside &lt;dir&gt; and must conform to the default S1000D issue (4.2).

-b &lt;BREX&gt;  
BREX data module code.

-C &lt;country&gt;  
The country ISO code of the new comment.

-c &lt;sec&gt;  
The security classification of the new comment.

-d &lt;defaults&gt;  
Specify the 'defaults' file name.

-f  
Overwrite existing file.

-I &lt;date&gt;  
The issue date of the new comment in the form of YYYY-MM-DD.

-L &lt;lang&gt;  
The language ISO code of the new comment.

-o &lt;orig&gt;  
The enterprise name of the originator of the comment.

-p  
Prompt the user for values left unspecified.

-q  
Do not report an error when the file already exists.

-r &lt;type&gt;  
The response type of the new comment.

-t &lt;title&gt;  
The title of the new comment.

-v  
Print the file name of the newly created comment.

'defaults' file
---------------

Refer to s1kd-newdm(1) for information on the 'defaults' file which is used by all the s1kd-new\* commands.

EXAMPLE
=======

    $ s1kd-newcom -# EX-12345-2018-00001-Q
