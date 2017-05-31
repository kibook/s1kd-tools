NAME
====

s1kd-upissue - Upissue an S1000D data module

SYNOPSIS
========

s1kd-upissue \[-vI\] &lt;datamodules&gt;

DESCRIPTION
===========

The *s1kd-upissue* tool increases the in-work or issue number of an S1000D data module.

OPTIONS
=======

-v  
Print the file name of the upissued data module.

-I  
Increase the issue number of the data module. By default, the in-work issue is increased.

-s &lt;status&gt;  
Set the status of the new issue. Default is 'changed'.
