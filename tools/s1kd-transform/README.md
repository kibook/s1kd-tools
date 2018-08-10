NAME
====

s1kd-transform - Apply XSL transformations to CSDB objects

SYNOPSIS
========

    s1kd-transform [-s <stylesheet> [-p <name>=<value> ...] ...]
                   [-o <file>] [-filh?] [<object> ...]

DESCRIPTION
===========

Applies an XSLT stylesheet to S1000D CSDB objects. The DTD of any specified objects is preserved in the resulting output, which leaves external entities such as ICN references intact.

OPTIONS
=======

-f  
Overwrite the specified CSDB object(s) instead of writing to stdout.

-h -?  
Show usage message.

-i  
Includes an "identity" template in to each specified stylesheet.

-l  
Treat input (stdin or arguments) as lists of CSDB objects to transform, rather than CSDB objects themselves.

-o &lt;file&gt;  
Output to &lt;file&gt; instead of stdout. This option only makes sense when the input is a single CSDB object.

-p &lt;name&gt;=&lt;value&gt;  
Pass a parameter to the last specified stylesheet.

-s &lt;stylesheet&gt;  
An XSLT stylesheet file to apply to each CSDB object. Multiple stylesheets can be specified by supplying this argument multiple times. The stylesheets will be applied in the order they are listed.

--version  
Show version information.

&lt;object&gt; ...  
Any number of CSDB objects to apply all specified stylesheets to.

Identity template
-----------------

The -i option includes an "identity" template in to each stylesheet specified with the -s option. The template is equivalent to this XSL:

    <xsl:template match="@*|node()">
    <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    </xsl:template>

This means that any attributes or nodes which are not matched by a more specific template in the user-specified stylesheet are copied.
