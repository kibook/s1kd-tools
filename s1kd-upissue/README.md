NAME
====

s1kd-upissue - Upissue S1000D data

SYNOPSIS
========

    s1kd-upissue [-viNrRqI] [-1 <type>] [-2 <type>] [-s <status>] <files>

DESCRIPTION
===========

The *s1kd-upissue* tool increases the in-work or issue number of an S1000D data module, publication module, etc.

Any files using an S1000D-esque naming convention, placing the issue and in-work numbers after the first underscore (\_) character, can also be "upissued". Files which do not contain the appropriate S1000D metadata are simply copied.

OPTIONS
=======

-1 &lt;type&gt;  
Set first verification type (tabtop, onobject, ttandoo).

-2 &lt;type&gt;  
Set second verification type (tabtop, onobject, ttandoo).

-I  
Do not change issue date. Normally, when upissuing to the next inwork or official issue, the issue date is changed to the current date. This option will keep the date of the previous inwork or official issue.

-i  
Increase the issue number of the data module. By default, the in-work issue is increased.

-N  
Omit issue/inwork numbers from filename.

-q  
Keep quality assurance information from old issue. Normally, when upissuing an official data module to the first in-work issue, the quality assurance is set back to "unverified". Specify this option to indicate the upissue will not affect the contents of the data module, and so does not require it to be re-verified.

-R  
Delete only change markup on elements associated with an RFU (by use of the attribute `reasonForUpdateRefIds`. Change markup on other elements is ignored.

-r  
Keep old RFUs. Normally, when upissuing an offical data module to the first in-work issue, any reasons for update are deleted automatically, along with any change markup attributes on elements. This option prevents their deletion.

-s &lt;status&gt;  
Set the status of the new issue. Default is 'changed'.

-v  
Print the file name of the upissued data module.

EXAMPLES
========

Data module with issue/inwork in filename
-----------------------------------------

    $ ls
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML

    $ s1kd-upissue DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    $ ls
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML

    $ s1kd-upissue \
      -i DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    $ ls
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_001-00_EN-CA.XML

Data module without issue/inwork in filename
--------------------------------------------

    $ ls
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-US.XML

    $ s1kd-metadata DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML \
      -n issueInfo
    000-01
    $ s1kd-upissue -N DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML
    $ s1kd-metadata DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML \
      -n issueInfo
    000-02

Non-XML file with issue/inwork in filename
------------------------------------------

    $ ls
    TXT-S1000DTOOLS-KHZAE-FOOBAR_000-01_EN-CA.TXT

    $ s1kd-upissue TXT-S1000DTOOLS-KHZAE-00001_000-01_EN-CA.TXT
    $ ls
    TXT-S1000DTOOLS-KHZAE-FOOBAR_000-01_EN-CA.TXT
    TXT-S1000DTOOLS-KHZAE-FOOBAR_000-02_EN-CA.TXT
