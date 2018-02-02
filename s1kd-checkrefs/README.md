NAME
====

s1kd-checkrefs - Check and update references in S1000D modules

SYNOPSIS
========

s1kd-checkrefs \[-s &lt;source&gt;\] \[-t &lt;target&gt;\] \[-d &lt;dir&gt;\] \[-cuFelvh?\] &lt;modules&gt;...

DESCRIPTION
===========

The *s1kd-checkrefs* tool takes a list of S1000D data modules and pub modules, and lists any invalid references to data/pub modules within them (references to modules not included in the list). It can also update the address items (title, issueDate if applicable) of all valid references using the corresponding address items of the given modules.

OPTIONS
=======

-s &lt;source&gt;  
Use only the specified module as the source of address items. Only references to this module will be checked and/or updated in all other modules.

-t &lt;target&gt;  
Only check and/or update references within this module. All other modules will only be used as sources.

-d &lt;dir&gt;  
Check references between data modules in the specified directory. Additional data modules can still be specified with -s.

-c  
Only check/update references within the content section of modules.

-u  
Update the address items of all valid references found within the specified modules.

-F  
Fail on first invalid reference and return an error code.

-e  
Check/update external publication references against a pre-defined list of publications.

-l  
List all invalid references found.

-v  
Verbose output.

-h -?  
Show help/usage message

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
