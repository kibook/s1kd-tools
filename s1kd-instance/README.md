NAME
====

s1kd-instance - Create S1000D data module instances

SYNOPSIS
========

s1kd-instance \[-s &lt;src&gt;\] \[-e &lt;ext&gt;\] \[-c &lt;dmc&gt;\] \[-l &lt;lang&gt;\] \[-o &lt;file&gt;|-O &lt;dir&gt;\] \[-f\] \[-t &lt;techName&gt;\] \[-i &lt;infoName&gt;\] \[-a|-A\] \[-Y &lt;text&gt;\] \[-C &lt;comment&gt;\] \[-R &lt;CIR&gt; ...\] \[-S\] \[&lt;applic&gt;...\]

DESCRIPTION
===========

The *s1kd-instance* tool filters a master S1000D data module on user-supplied applicability definitions, producing a new data module instance with non-applicable elements and (optionally) unused applicability statements removed.

OPTIONS
=======

-s &lt;src&gt;  
The source data module (default is to read from stdin).

-e &lt;ext&gt;  
Specify an extension on the data module code (DME) for the instance.

-c &lt;dmc&gt;  
Specify a new data module code (DMC) for the instance.

-l &lt;lang&gt;  
Set the language and country of the data module instance. For example, to create an instance for US English, lang would be "en-US".

-o &lt;file&gt;  
Output data module instance to file instead of stdout.

-O &lt;dir&gt;  
Output data module instance in dir, automatically naming it based on:

-   the extension specified with -e, and/or

-   the data module code specified with -c, and/or

-   the language and country specified with -L

The issue information is copied from the source data module.

-f  
Overwrite existing file with same name as the filename generated automatically with -O, if it exists.

-t &lt;techName&gt;  
Give the data module instance a different techName.

-i &lt;infoName&gt;  
Give the data module instance a different infoName.

-a  
Remove unused applicability annotations but not statements.

-A  
Remove unused applicability annotations and simplify/remove unused applicability statements.

-Y &lt;text&gt;  
Set the applicability for the whole data module using the user-definied applicability values, using text as the new display text.

-C &lt;comment&gt;  
Add an XML comment to the top of the data module instance. Useful as another way of identifying a data module as an instance aside from the source data module address or extended data module code, or giving additional information about a particular instance.

-R &lt;CIR&gt; ...  
Use a CIR to resolve external dependencies in the master data module, making the instance data module standalone. Additional CIRs can be used by specifying the -R option multiple times. Currently the functional item, warnings/cautions and applicability CIRs are supported.

-S  
Do not include &lt;sourceDmIdent&gt; in the data module instance.

&lt;applic&gt;...  
Any number of applicability definitions in the form of: &lt;ident&gt;:&lt;type&gt;=&lt;value&gt;

-a vs -A
--------

The -a option will remove applicability annotations (applicRefId) from elements which are deemed to be unambigously valid (their validity does not rely on applicability values left undefined by the user). The applicability statements themselves however will be untouched.

The -A option will do the above, but will also attempt to simplify unused parts of applicability statements or remove unused applicability statements entirely. It simplifies a statement by removing &lt;assert&gt; elements determined to be either unambigously valid or invalid given the user-definied values, and removing uneeded &lt;evaluate&gt; elements when they contain only one remaining &lt;assert&gt;.

> **Note**
>
> The -A option may change the *meaning* of certain applicability statements without changing the *display text*. Display text is always left untouched, so using this option may cause display text to be technically incorrect.

Identifying source data module of an instance
---------------------------------------------

The resulting data module instance will contain the element &lt;sourceDmIdent&gt;, which will contain a &lt;dmRef&gt; to the data module specified with the -s option.

Additionally, the instance will contain an element &lt;repositorySourceDmIdent&gt; for each CIR specified with the -R option.

If the -S option is used, neither the &lt;sourceDmIdent&gt; element or &lt;repositorySourceDmIdent&gt; elements are added. This can be useful when this tool is not used to make an "instance" per say, but more generally to make a data module based on an existing data module.

Instance data module code (-c) vs extension (-e)
------------------------------------------------

When creating a data module instance, the instance should have the same data module code as the master data module, with an added extension code, the DME. However, in cases where a vendor does not support this extension or possibly when this tool is used to create "instances" which will from that point on be maintained as normal standalone data modules, it may be desirable or necessary to change the data module code instead. These two options can be used together as well to give an instance a new DMC as well an extension.
