NAME
====

s1kd-upissue - Upissue S1000D data

SYNOPSIS
========

s1kd-upissue \[-viN\] \[-s &lt;status&gt;\] &lt;files&gt;

DESCRIPTION
===========

The *s1kd-upissue* tool increases the in-work or issue number of an S1000D data module, publication module, etc.

Any files using an S1000D-esque naming convention, placing the issue and in-work numbers after the first underscore (\_) character, can also be "upissued". Files which do not contain the appropriate S1000D metadata are simply copied.

OPTIONS
=======

-v  
Print the file name of the upissued data module.

-i  
Increase the issue number of the data module. By default, the in-work issue is increased.

-s &lt;status&gt;  
Set the status of the new issue. Default is 'changed'.

-N  
Omit issue/inwork numbers from filename.

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

    $ s1kd-upissue -i DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    $ ls
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_001-00_EN-CA.XML

Data module without issue/inwork in filename
--------------------------------------------

    $ ls
    DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-US.XML

    $ s1kd-metadata DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML issueInfo
    000-01
    $ s1kd-upissue -N DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML
    $ s1kd-metadata DMC-S1000DTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML issueInfo
    000-02

Non-XML file with issue/inwork in filename
------------------------------------------

    $ ls
    TXT-S1000DTOOLS-KHZAE-FOOBAR_000-01_EN-CA.TXT

    $ s1kd-upissue TXT-S1000DTOOLS-KHZAE-00001_000-01_EN-CA.TXT
    $ ls
    TXT-S1000DTOOLS-KHZAE-FOOBAR_000-01_EN-CA.TXT
    TXT-S1000DTOOLS-KHZAE-FOOBAR_000-02_EN-CA.TXT
