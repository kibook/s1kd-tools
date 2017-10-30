NAME
====

s1kd-acronyms - Generate a list of acronyms from S1000D data modules

SYNOPSIS
========

s1kd-acronyms \[-pxdtiIh?\] \[-n &lt;\#&gt;\] \[-T &lt;types&gt;\] \[-m &lt;acronyms&gt;\] \[-o &lt;file&gt;\] \[&lt;datamodules&gt;\]

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

-i -I  
Markup acronyms in interactive mode. If the specified acronyms list contains multiple definitions for a given acronym term, the tool will prompt the user with the context in which the acronym is used and present a list of the definitions for them to choose from.

When not in interactive mode, the first definition found will be used.

The -I option prompts for all acronyms, not just those with multiple definitions. This can be useful if some occurrences of the acronym term should be ignored.

-o &lt;file&gt;  
Output to &lt;file&gt; instead of stdout. When used with the -m option, output to &lt;file&gt; instead of overwriting the existing file.

-h -?  
Show help/usage message.

&lt;datamodules&gt;  
Data modules to find acronyms in.
