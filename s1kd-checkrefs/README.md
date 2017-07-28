NAME
====

s1kd-checkrefs - Check and update references in S1000D modules

SYNOPSIS
========

s1kd-checkrefs \[-s &lt;source&gt;\] \[-t &lt;target&gt;\] \[-cuFvh?\] &lt;modules&gt;...

DESCRIPTION
===========

The *s1kd-checkrefs* tool takes a list of S1000D data modules and pub modules, and lists any invalid references to data/pub modules within them (references to modules not included in the list). It can also update the address items (title, issueDate if applicable) of all valid references using the corresponding address items of the given modules.

OPTIONS
=======

-s &lt;source&gt;  
Use only the specified module as the source of address items. Only references to this module will be checked and/or updated in all other modules.

-t &lt;target&gt;  
Only check and/or update references within this module. All other modules will only be used as sources.

-c  
Only check/update references within the content section of modules.

-u  
Update the address items of all valid references found within the specified modules.

-F  
Fail on first invalid reference and return an error code.

-v  
Verbose output.

-h -?  
Show help/usage message
