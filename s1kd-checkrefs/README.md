NAME
====

s1kd-checkrefs - Check and update references in S1000D modules

SYNOPSIS
========

    s1kd-checkrefs [-s <source>] [-t <target>] [-m <object>]
                   [-d <dir>] [-cuFfeLlvh?] <modules>...

DESCRIPTION
===========

The *s1kd-checkrefs* tool takes a list of S1000D data modules and pub modules, and lists any invalid references to data/pub modules within them (references to modules not included in the list). It can also update the address items (title, issueDate if applicable) of all valid references using the corresponding address items of the given modules.

OPTIONS
=======

-c  
Only check/update references within the content section of modules.

-d &lt;dir&gt;  
Check references between data modules in the specified directory. Additional data modules can still be specified with -s.

-e  
Check/update external publication references against a pre-defined list of publications.

-F  
Fail on first invalid reference and return an error code.

-f  
List files which contain invalid references.

-h -?  
Show help/usage message

-L  
Treat input as a list of data module filenames, rather than a data module itself.

-l  
List all invalid references found.

-m &lt;object&gt;  
Change all references to the source object specified with -s into references that point to &lt;object&gt;.

-s &lt;source&gt;  
Use only the specified module as the source of address items. Only references to this module will be checked and/or updated in all other modules.

-t &lt;target&gt;  
Only check and/or update references within this module. All other modules will only be used as sources.

-u  
Update the address items of all valid references found within the specified modules.

-v  
Verbose output.

External publication list (-e)
------------------------------

Since external publications can be of any format, in order to check references to them, their metadata must be specified in an XML format for the s1kd-checkrefs tool to read.

The root element of the XML file is the `externalPubs` element. Each external publication is represented by an element `externalPubAddress`. The identifying elements of the publication are stored in the `externalPubIdent` element (corresponding with the `externalPubRefIdent` element). The address items are stored in the `externalPubAddress` element (corresponding with the `externalPubRefAddressItems` element).

Example:

    <?xml version="1.0"?>
    <externalPubs>
    <externalPubAddress>
    <externalPubIdent>
    <externalPubCode>s1kd-checkrefs</externalPubCode>
    <externalPubTitle>s1kd-checkrefs manual</externalPubTitle>
    </externalPubIdent>
    <externalPubAddressItems>
    <externalPubIssueDate year="2017" month="08" day="14"/>
    </externalPubAddressItems>
    </externalPubAddress>
    </externalPubs>

EXAMPLES
========

Validate all references in all data modules:

    $ s1kd-checkrefs DMC-*.XML

Validate references in a single data module:

    $ s1kd-checkrefs -t <DM> DMC-*.XML

Update all references in all data modules:

    $ s1kd-checkrefs -u DMC-*.XML

Change references from one data module to another in all data modules:

    $ s1kd-checkrefs -s <old DM> -m <new DM> DMC-*.XML
