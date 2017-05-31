NAME
====

s1kd-brexcheck - Validate S1000D data modules against BREX data modules

SYNOPSIS
========

s1kd-brexcheck \[-b &lt;brex&gt;\] \[-I &lt;path&gt; \[-vh?\] &lt;datamodules&gt;

DESCRIPTION
===========

The *s1kd-brexcheck* tool validates an S1000D data module using the context rules of one or multiple Business Rule EXchange (BREX) data modules. All errors are displayed with the &lt;objectUse&gt; message, the line number, and a representation of the invalid XML tree.

OPTIONS
=======

-b &lt;brex&gt;  
Check the data modules against this BREX. Multiple BREX data modules can be specified by adding this option multiple times. When no BREX data modules are specified, the BREX data module referenced in &lt;brexDmRef&gt; in the data module is attempted to be used instead.

-I &lt;path&gt;  
Add a search path for BREX data modules. By default, only the current directory is searched.

-v  
Use verbose output.

-s  
Use shortened, single-line messages to report BREX errors instead of multiline indented messages.

-x  
Output an XML report instead of a plain-text one.

-h -?  
Show the help/usage message.
