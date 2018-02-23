NAME
====

s1kd-transform - Apply XSL transformations to data modules

SYNOPSIS
========

    s1kd-transform [-s <stylesheet> ...] [-o <file>] [-ifh?] [<data module> ...]

DESCRIPTION
===========

Applies an XSLT stylesheet to S1000D data modules. The DTD of any specified data modules is preserved in the resulting output.

OPTIONS
=======

-f  
Overwrite the specified data module(s) instead of writing to stdout.

-h -?  
Show usage message.

-i  
Includes an "identity" template in to each specified stylesheet.

-o &lt;file&gt;  
Output to &lt;file&gt; instead of stdout. This option only makes sense when the input is a single data module.

-s &lt;stylesheet&gt;  
An XSLT stylesheet file to apply to each data module. Multiple stylesheets can be specified by supplying this argument multiple times. The stylesheets will be applied in the order they are listed.

&lt;data module&gt; ...  
Any number of data modules to apply all specified stylesheets to.

Identity template
-----------------

The -i option includes an "identity" template in to each stylesheet specified with the -s option. The template is equivalent to this XSL:

    <xsl:template match="@*|node()">
    <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
    </xsl:template>

This means that any attributes or nodes which are not matched by a more specific template in the user-specified stylesheet are copied.
