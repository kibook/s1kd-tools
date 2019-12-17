NAME
====

s1kd-acronyms - Manage acronyms in S1000D data modules

SYNOPSIS
========

    s1kd-acronyms -h?
    s1kd-acronyms [-dlptvx] [-n <#>] [-o <file>] [-T <types>]
                  [<dmodule>...]
    s1kd-acronyms [-flv] [-i|-I|-!] [-m|-M <acr>] [-o <file>] [-X <xpath>]
                  [<dmodule>...]
    s1kd-acronyms [-D|-P] [-flv] [-o <file>] [<dmodule>...]

DESCRIPTION
===========

The *s1kd-acronyms* tool is used to manage acronyms in S1000D data
modules in one of three ways:

-   Generate a list of unique acronyms used in all specified data
    modules.

-   Mark up acronyms automatically based on a specified list.

-   Remove acronym markup.

OPTIONS
=======

-D, --delete  
Remove acronym markup, flattening it to the acronym term.

-d, --deflist  
Format XML output as an S1000D `<definitionList>`.

-f, --overwrite  
When marking up acronyms with the -m option, overwrite the input data
modules instead of writing to stdout.

-h, -?, --help  
Show help/usage message.

-I, --always-ask  
In interactive mode, show a prompt for all acronyms, not just those with
multiple definitions. This can be useful if some occurrences of acronym
terms should be ignored.

-i, --interactive  
Markup acronyms in interactive mode. If the specified acronyms list
contains multiple definitions for a given acronym term, the tool will
prompt the user with the context in which the acronym is used and
present a list of the definitions for them to choose from.

When not in interactive mode, the first definition found will be used.

-l, --list  
Treat input (stdin or arguments) as lists of filenames of data modules
to find or markup acronyms in, rather than data modules themselves.

-M, --acronym-list &lt;list&gt;  
Like the -m option, but use a custom list of acronyms instead of the
default `.acronyms` file.

-m, --markup  
Instead of listing acronyms in the specified data modules, automatically
markup acronyms in the data module using the `.acronyms` file.

-n, --width &lt;\#&gt;  
Minimum number of spaces after the term in pretty-printed text output.

-o, --out &lt;file&gt;  
Output to &lt;file&gt; instead of stdout.

-P, --preformat  
Remove acronym markup by preformatting it. The element `<acronym>` is
flattened to the definition, followed by the term in brackets \[()\].
The element `<acronymTerm>` is flattened to the term.

-p, --pretty  
Pretty print text/XML acronym list output.

-T, --types &lt;types&gt;  
Only search for acronyms with an attribute `acronymType` whose value is
contained within the string &lt;types&gt;.

-t, --table  
Format XML output as an S1000D `<table>`.

-v, --verbose  
Verbose output.

-X, --select &lt;xpath&gt;  
When marking up acronyms with -m/-M, use a custom XPath expression to
specify which text nodes to search for acronyms in. By default, this is
all text nodes in any element where acronyms are allowed. This must be
the path to the text() nodes, not the elements, e.g. `//para/text()` and
not simply `//para`.

-x, --xml  
Use XML output instead of plain text.

-!, --defer-choice  
Mark where acronyms are found using a `<chooseAcronym>` element, whose
child elements are all possible acronyms matching the term. Another
program can then use this as input to actually prompt the user.

--version  
Show version information.

&lt;dmodule&gt;...  
Data modules to find acronyms in. If none are specified, input is taken
from stdin.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--xinclude  
Do XInclude processing.

`.acronyms` file
----------------

This file specifies a list of acronyms for a project. By default, the
program will search for a file named `.acronyms` in the current
directory and parent directories, but any file can be specified using
the -M option.

Example of .acronyms file format:

    <acronyms>
    <acronym acronymType="at01">
    <acronymTerm>BREX</acronymTerm>
    <acronymDefinition>Business Rules Exchange</acronymDefinition>
    </acronym>
    <acronym acronymType="at01">
    <acronymTerm>SNS</acronymTerm>
    <acronymDefinition>Standard Numbering System</acronymDefinition>
    </acronym>
    </acronyms>

EXAMPLES
========

List all acronyms used in all data modules:

    $ s1kd-acronyms DMC-*.XML

Markup predefined acronyms in a data module:

    $ s1kd-acronyms -mf DMC-EX-A-00-00-00-00A-040A-D_EN-CA.XML

Unmarkup acronyms in a data module:

    $ s1kd-acronyms -Df DMC-EX-A-00-00-00-00A-040A-D_EN-CA.XML
