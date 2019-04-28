NAME
====

s1kd-newupf - Create a new data update file

SYNOPSIS
========

    s1kd-newupf [options] <SOURCE> <TARGET>

DESCRIPTION
===========

The *s1kd-newupf* tool creates a new S1000D data update file for two
specified issues of a CIR data module. Changes to items between the
source and target issues of the CIR are recorded in the resulting UPF,
along with update instructions.

OPTIONS
=======

-$ &lt;issue&gt;  
Specify which issue of S1000D to use. Currently supported issues are:

-   4.2 (default)

-   4.1

-@ &lt;path&gt;  
Save the new update file to &lt;path&gt;. If &lt;path&gt; is an existing
directory, the update file will be created in it instead of the current
directory. Otherwise, the update file will be saved as the filename
&lt;path&gt; instead of being automatically named.

-% &lt;dir&gt;  
Use XML template in the specified directory instead of the built-in
template. The template must be named `update.xml` in the directory
&lt;dir&gt;, and must conform to the default S1000D issue of this tool
(4.2).

-\~ &lt;dir&gt;  
Dump the built-in XML template to the specified directory.

-d &lt;defaults&gt;  
Specify the `.defaults` file name.

-f  
Overwrite existing file.

-h -?  
Show help/usage message.

-q  
Do not report an error when the file already exists.

-v  
Print the file name of the newly created data update file.

--version  
Show version information.

&lt;SOURCE&gt;  
The source (original) issue of the CIR data module.

&lt;TARGET&gt;  
The target (updated) issue of the CIR data module.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

`.defaults` file
----------------

Refer to s1kd-newdm(1) for information on the `.defaults` file which is
used by all the s1kd-new\* tools.

EXAMPLE
=======

    $ s1kd-newupf \
        DMC-EX-A-00-00-00-00A-00GA-D_001-00_EN-CA.XML \
        DMC-EX-A-00-00-00-00A-00GA-D_002-00_EN-CA.XML
