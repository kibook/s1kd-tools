NAME
====

s1kd-acronyms - Generate a list of acronyms from S1000D data modules

SYNOPSIS
========

s1kd-acronyms \[-pxdth?\] \[-n &lt;\#&gt;\] \[-T &lt;types&gt;\] \[-o &lt;file&gt;\] \[&lt;datamodules&gt;\]

DESCRIPTION
===========

The *s1kd-acronyms* tool generates a list of unique acronyms used in S1000D data modules.

OPTIONS
=======

-p  
Pretty print text/XML output.

-x  
Use XML output instead of plain text.

-d  
Format XML output as an S1000D `<definitionList>`.

-t  
Format XML output as an S1000D `<table>`.

-n &lt;\#&gt;  
Minimum number of spaces after the term in pretty-printed text output.

-T &lt;types&gt;  
Only search for acronyms with an attribute `acronymType` whose value is contained within the string &lt;types&gt;.

-m &lt;acronyms&gt;  
Instead of listing acronyms, automatically markup acronyms given in the &lt;acronyms&gt; XML file in the specified data modules. Occurrences of the acronym term will be replaced in text with the `acronym` element in the list.

-o &lt;file&gt;  
Output to &lt;file&gt; instead of stdout. When used with the -m option, output to &lt;file&gt; instead of overwriting the existing file.

-h -?  
Show help/usage message.

&lt;datamodules&gt;  
Data modules to find acronyms in.
