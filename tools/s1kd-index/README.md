NAME
====

s1kd-index - Flag index keywords in a data module

SYNOPSIS
========

    s1kd-index -h?
    s1kd-index [-I <index>] [-fi] [<module>...]
    s1kd-index -D [-fi] [<module>...]

DESCRIPTION
===========

The *s1kd-index* tool adds index flags to a data module based on a user-defined set of keywords.

OPTIONS
=======

-D  
Remove the current index flags from a data module.

-f  
Overwrite input module(s).

-I &lt;index&gt;  
Flag the terms in the specified &lt;index&gt; XML file instead of the default `.indexflags` file.

-i  
Ignore case when flagging terms.

-h -?  
Show help/usage message.

--version  
Show version information.

`.indexflags` file
------------------

This file specifies the list of indexable keywords for the project and their level. By default, the program will search for a file named `.indexflags` in the current directory, but any file can be specified using the -I option.

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
