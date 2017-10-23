NAME
====

s1kd-dmrl - Create CSDB objects from a DMRL

SYNOPSIS
========

s1kd-dmrl \[-Nh?\] &lt;DML&gt;...

DESCRIPTION
===========

The *s1kd-dmrl* tool reads S1000D data management lists and creates CSBD objects for the entries specified using the s1kd-new\* tools.

OPTIONS
=======

-s  
Do not create CSDB objects, only output the s1kd-new\* commands to create them.

-N  
Omit issue/in-work numbers from the filenames of created CSDB objects.

-h -?  
Show help/usage message.

&lt;DML&gt;...  
One or more S1000D data management lists.
