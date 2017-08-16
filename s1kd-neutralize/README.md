NAME
====

s1kd-neutralize - S1000D IETP neutral translation of data modules

SYNOPSIS
========

s1kd-neutralize \[-o &lt;file&gt;\] \[-rh?\] &lt;datamodules&gt;

DESCRIPTION
===========

Generates neutral metadata for the specified data modules. This includes:

-   XLink attributes for references, using the S1000D URN scheme.

-   RDF and Dublin Core metadata.

OPTIONS
=======

-o &lt;file&gt;  
Output neutralized data module XML to &lt;file&gt; instead of overwriting the source data module.

-h -?  
Show usage message.
