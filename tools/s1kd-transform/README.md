NAME
====

s1kd-transform - Apply XSL transformations to CSDB objects

SYNOPSIS
========

    s1kd-transform [-s <stylesheet> [-p <name>=<value> ...] ...]
                   [-o <file>] [-filqvh?] [<object> ...]

DESCRIPTION
===========

Applies an XSLT stylesheet to S1000D CSDB objects. The DTD of any
specified objects is preserved in the resulting output, which leaves
external entities such as ICN references intact.

OPTIONS
=======

-f, --overwrite  
Overwrite the specified CSDB object(s) instead of writing to stdout.

-h, -?, --help  
Show usage message.

-i, --identity  
Includes an "identity" template in to each specified stylesheet.

-l, --list  
Treat input (stdin or arguments) as lists of CSDB objects to transform,
rather than CSDB objects themselves.

-o, --out &lt;file&gt;  
Output to &lt;file&gt; instead of stdout. This option only makes sense
when the input is a single CSDB object.

-p, --param &lt;name&gt;=&lt;value&gt;  
Pass a parameter to the last specified stylesheet.

-q, --quiet  
Quiet mode. Errors are not printed.

-s, --stylesheet &lt;stylesheet&gt;  
An XSLT stylesheet file to apply to each CSDB object. Multiple
stylesheets can be specified by supplying this argument multiple times.
The stylesheets will be applied in the order they are listed.

-v, --verbose  
Verbose output.

--version  
Show version information.

&lt;object&gt; ...  
Any number of CSDB objects to apply all specified stylesheets to.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--huge  
Remove any internal arbitrary parser limits.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

--parser-errors  
Emit errors from parser.

--parser-warnings  
Emit warnings from parser.

--xinclude  
Do XInclude processing.

Identity template
-----------------

The -i option includes an "identity" template in to each stylesheet
specified with the -s option. The template is equivalent to this XSL:

    <xsl:template match="@*|node()">
    <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    </xsl:template>

This means that any attributes or nodes which are not matched by a more
specific template in the user-specified stylesheet are copied.

EXAMPLE
=======

    $ s1kd-transform -s <XSL> <DM1> <DM2> ...
