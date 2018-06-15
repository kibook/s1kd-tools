NAME
====

s1kd-sns - Organize data modules based on an SNS

SYNOPSIS
========

    s1kd-sns [-d <dir>] [-npsh?] [<BREX> ...]

DESCRIPTION
===========

The *s1kd-sns* tool can be used to automatically organize data modules in a CSDB in to a directory hierarchy based on a specified SNS structure. It may also be used to simply print an indented text version of an SNS structure.

OPTIONS
=======

-d &lt;dir&gt;  
The root directory of the new SNS structure. By default, the tool will use the name "SNS" in the current directory.

-h -?  
Show usage message.

-n  
Use only the SNS codes when naming directories. By default, each directory will be named in the form of "snsCode - snsTitle".

-p  
Print the SNS structure only.

-s  
Use symbolic links to organize the SNS instead of the default hard links.

&lt;BREX&gt;  
Read the SNS structure from the specified BREX data module. If none is specified, the tool will read from stdin.

EXAMPLE
=======

    $ s1kd-sns DMC-S1000D-A-08-02-0100-00A-022A-D_EN-US.XML
    $ tree SNS
    SNS
    |_ 00 - Product, General
       |_ 0 - Product, General
       |_ 1 - Product, General maintenance
       |_ 2 - Product, Safety
       |
    ...
    |_ 04 - Worthiness (fit for purpose) limitations
       |_ 0 - General
       |_ 1 - Fatigue index calculations
       |_ 2 - Operating spectrums
    |_ 05 - Scheduled/unscheduled maintenance
       |_ 0 - General
       |_ 1 - Time limits
       |_ 2 - Scheduled maintenance check lists
    ...
    |_ 18 - Vibration and noise analysis and attenuation
