NAME
====

s1kd-index - Flag index keywords in a data module

SYNOPSIS
========

    s1kd-index -h?
    s1kd-index [-I <index>] [-filv] [<module>...]
    s1kd-index -D [-filv] [<module>...]

DESCRIPTION
===========

The *s1kd-index* tool adds index flags to a data module based on a
user-defined set of keywords.

OPTIONS
=======

-D  
Remove the current index flags from a data module.

-f  
Overwrite input module(s).

-I &lt;index&gt;  
Flag the terms in the specified &lt;index&gt; XML file instead of the
default `.indexflags` file.

-i  
Ignore case when flagging terms.

-l  
Treat input (stdin or arguments) as lists of filenames of data modules
to add index flags to, rather than data modules themselves.

-v  
Verbose output.

-h -?  
Show help/usage message.

--version  
Show version information.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

`.indexflags` file
------------------

This file specifies the list of indexable keywords for the project and
their level. By default, the program will search for a file named
`.indexflags` in the current directory or parent directories, but any
file can be specified using the -I option.

Exmaple of `.indexflags` file format:

    <indexFlags>
    <indexFlag indexLevelOne="bicycle"/>
    <indexFlag indexLevelOne="bicycle" indexLevelTwo="brake system"/>
    </indexFlags>

EXAMPLE
=======

Given the following in a data module:

    <levelledPara>
    <title>General</title>
    <para>
    The s1kd-tools are a set of small tools for manipulating S1000D XML
    data.
    </para>
    </levelledPara>

And the following `.indexflags` file:

    <indexFlags>
    <indexFlag indexLevelOne="S1000D"/>
    <indexFlag indexLevelTwo="S10000D" indexLevelTwo="s1kd-tools"/>
    <indexFlag indexLevelOne="data"/>
    <indexFlag indexLevelOne="data" indexLevelTwo="XML"/>
    </indexFlags>

Then the s1kd-index command:

    $ s1kd-index <DM>.XML

Would result in the following:

    <levelledPara>
    <title>General</title>
    <para>
    The s1kd-tools<indexFlag indexLevelOne="S1000D"
    indexLevelTwo="s1kd-tools"/> are a set of small tools for
    manipulating S1000D<indexFlag indexLevelOne="S1000D"/>
    XML<indexFlag indexLevelOne="data" indexLevelTwo="XML"/>
    data<indexFlag indexLevelOne="data"/>.
    </para>
    </levelledPara>
