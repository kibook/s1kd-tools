NAME
====

s1kd-upissue - Upissue S1000D data

SYNOPSIS
========

    s1kd-upissue [-DdefHIilmNqRruvw] [-1 <type>] [-2 <type>]
                 [-c <reason>] [-s <status>] [-t <urt>] [<file>...]

DESCRIPTION
===========

The *s1kd-upissue* tool increases the in-work or issue number of an
S1000D CSDB object.

Any files using an S1000D-esque naming convention, placing the issue and
in-work numbers after the first underscore (\_) character, can also be
"upissued". Files which do not contain the appropriate S1000D metadata
are simply copied.

OPTIONS
=======

-1 &lt;type&gt;  
Set first verification type (tabtop, onobject, ttandoo).

-2 &lt;type&gt;  
Set second verification type (tabtop, onobject, ttandoo).

-c &lt;reason&gt;  
Add a reason for update to the upissued objects. Multiple RFUs can be
added by specifying this option multiple times.

-D  
Remove elements with change type of "delete". These elements are
automatically removed along with all change marks and RFUs when an
object is upissued from official to the next inwork issue. This option
will remove them when upissuing between inwork issues, or when making
the object official.

-d  
Do not actually create or modify any files, only print the name of the
file that would be created or modified.

-e  
Remove old issue file after upissuing.

-f  
Overwrite existing upissued CSDB objects.

-H  
Mark the last specified reason for update (-c) as a highlight.

-I  
Do not change issue date. Normally, when upissuing to the next inwork or
official issue, the issue date is changed to the current date. This
option will keep the date of the previous inwork or official issue.

-i  
Increase the issue number of the CSDB object. By default, the in-work
issue is increased.

-l  
Treat input (stdin or arguments) as lists of CSDB objects to upissue,
rather than CSDB objects themselves.

-m  
Modify issue-related metadata on objects without incrementing the issue
or inwork numbers. The -I, -q, and -r options have the opposite effect
in this mode. The modified objects are written to stdout by default, and
the -f option can be used to change them in-place.

-N  
Omit issue/inwork numbers from filename.

-q  
Keep quality assurance information from old issue. Normally, when
upissuing an official CSDB object to the first in-work issue, the
quality assurance is set back to "unverified". Specify this option to
indicate the upissue will not affect the contents of the CSDB object,
and so does not require it to be re-verified.

-R  
Delete only change markup on elements associated with an RFU (by use of
the attribute `reasonForUpdateRefIds`). Change markup on other elements
is ignored.

-r  
Keep old RFUs. Normally, when upissuing an offical CSDB object to the
first in-work issue, any reasons for update are deleted automatically,
along with any change markup attributes on elements (when change type is
"add" or "modify") or the elements themselves (when change type is
"delete"). This option prevents their deletion.

-s &lt;status&gt;  
Set the status of the new issue. Default is 'changed'.

-t &lt;urt&gt;  
Set the updateReasonType of the last specified reason for update (-c).

-u  
Remove RFUs which are not associated with any change markup (by use of
the attribute `reasonForUpdateRefIds`).

-v  
Print the file name of the upissued CSDB object.

-w  
Make the old issue file read-only after upissuing. Official issues (-i)
will also be made read-only when they are created.

--version  
Show version information.

&lt;file&gt;...  
Any number of CSDB objects or other files to upissue. If none are
specified, the object will be read from stdin and the upissued object
will be written to stdout.

In addition, the following options enable features of the XML parser
that are disabled as a precaution by default:

--dtdload  
Load the external DTD.

--net  
Allow network access to load external DTD and entities.

--noent  
Resolve entities.

EXAMPLES
========

Data module with issue/inwork in filename
-----------------------------------------

    $ ls
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML

    $ s1kd-upissue DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    $ ls
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML

    $ s1kd-upissue \
      -i DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    $ ls
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_001-00_EN-CA.XML

Data module without issue/inwork in filename
--------------------------------------------

    $ ls
    DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_EN-US.XML

    $ s1kd-metadata DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML \
      -n issueInfo
    000-01
    $ s1kd-upissue -N DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML
    $ s1kd-metadata DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML \
      -n issueInfo
    000-02

Non-XML file with issue/inwork in filename
------------------------------------------

    $ ls
    TXT-S1KDTOOLS-KHZAE-FOOBAR_000-01_EN-CA.TXT

    $ s1kd-upissue TXT-S1KDTOOLS-KHZAE-00001_000-01_EN-CA.TXT
    $ ls
    TXT-S1KDTOOLS-KHZAE-FOOBAR_000-01_EN-CA.TXT
    TXT-S1KDTOOLS-KHZAE-FOOBAR_000-02_EN-CA.TXT
