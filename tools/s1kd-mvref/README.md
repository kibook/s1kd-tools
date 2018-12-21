NAME
====

s1kd-mvref - Change one reference in to another in S1000D CSDB objects

SYNOPSIS
========

    s1kd-mvref [-d <dir>] [-s <source>] [-t <target>] [-cflvh?]
               [<object>...]

DESCRIPTION
===========

The *s1kd-mvref* tool changes all references to one object (the source
object) into references to another object (the target object) in a
specified set of objects.

OPTIONS
=======

-c  
Only move references within the content section of objects.

-d &lt;dir&gt;  
Move references in all objects in the specified directory.

-f  
Overwrite updated input objects.

-h -?  
Show help/usage message

-l  
Treat input as a list of data module filenames, rather than a data
module itself.

-s &lt;source&gt;  
The source object.

-t &lt;target&gt;  
Change all references to the source object specified with -s into
references that point to &lt;target&gt;.

-v  
Verbose output.

--version  
Show version information.

&lt;object&gt;...  
Objects to move references in.

EXAMPLE
=======

    $ s1kd-mvref -f -s <old DM> -t <new DM> DMC-*.XML
