NAME
====

s1kd-index - Flag index keywords in a data module

SYNOPSIS
========

    s1kd-index [-fih?] [-I <index>] [<module>...]

DESCRIPTION
===========

The *s1kd-index* tool adds index flags to a data module based on a user-defined set of keywords.

OPTIONS
=======

-f  
Overwrite input module(s).

-I &lt;index&gt;  
Flag the terms in the specified &lt;index&gt; XML file.

-i  
Ignore case when flagging terms.

-h -?  
Show help/usage message.

--version  
Show version information.

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

And the following index file:

    <index>
    <indexFlag indexLevelOne="S1000D"/>
    <indexFlag indexLevelTwo="S10000D" indexLevelTwo="s1kd-tools"/>
    <indexFlag indexLevelOne="data"/>
    <indexFlag indexLevelOne="data" indexLevelTwo="XML"/>
    </index>

Then the s1kd-index command:

    $ s1kd-index -I <INDEX>.XML <DM>.XML

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
