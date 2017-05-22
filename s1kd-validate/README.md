NAME
====

s1kd-validate - Validate an S1000D data module against its schema

SYNOPSIS
========

s1kd-validate \[-d &lt;dir&gt;\] \[-vqD\] \[&lt;datamodules&gt;\]

DESCRIPTION
===========

The *s1kd-validate* tool validates an S1000D data module, checking whether it is a valid XML file and if it is valid against its own S1000D schema.

OPTIONS
=======

-d &lt;dir&gt;  
Search for schemas in &lt;dir&gt;. Normally, the URI of the schema is used to fetch it locally or over a network, but this option will force searching to be performed only in the specified directory.

-v -q -D  
Set the verbosity of the output, verbose, quiet, and debug. Verbose will explictly indicate success, rather than simply not displaying any errors. Quiet will not output anything.

&lt;datamodules&gt;  
Any number of data modules to validate.
