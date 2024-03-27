# NAME

s1kd-ls - List CSDB objects in a directory hierarchy

# SYNOPSIS

    s1kd-ls [-0CDGIiLlMNnoPRrSUwX7] [-e <cmd>]
            [<object>|<dir> ...]

# DESCRIPTION

The *s1kd-ls* tool searches the current directory or specified directory
trees and lists the file names of CSDB objects matching certain
criteria.

The files representing the CSDB objects must use either the standard
S1000D file naming conventions, or the alternate naming convention
supported by these tools using the -N option.

# OPTIONS

  - \-0, --null  
    Output a null-delimited list of CSDB object paths.

  - \-C, -D, -G, -L, -M, -P, -S, -U, -X  
    List comments, data modules, ICNs, data management lists, ICN
    metadata files, publication modules, SCORM content packages, data
    update files, and data dispatch notes respectively. If none are
    specified, -CDGLMPSUX is assumed.
    
    The following long options can also be used for each: --com, --dm,
    --icn, --dml, --imf, --pm, --smc, --upf, --ddn.

  - \-e, --exec \<cmd\>  
    Execute a command for each CSDB object instead of listing them. The
    string "{}" is replaced by the current CSDB object file name
    everywhere it occurs in the arguments to the command.

  - \-h, -?, --help  
    Show the usage message.

  - \-I, --inwork  
    Show only inwork issues of objects (inwork \!= 00).

  - \-i, --official  
    Show only official issues of objects (inwork = 00).

  - \-l, --latest  
    Show only the latest official/inwork issue of objects.

  - \-N, --omit-issue  
    Assume that the files being listed do not include the issue info in
    their filenames, i.e. they were created using the -N option of the
    s1kd-new\* tools.

  - \-n, --other  
    List non-S1000D files.

  - \-o, --old  
    Show only old official/inwork issues of objects.

  - \-R, --read-only  
    Show only non-writable object files.

  - \-r, --recursive  
    Recursively descend in to directories.

  - \-w, --writable  
    Show only writable object files.

  - \-7, --list  
    Treat input as a list of CSDB objects to process.

  - \--version  
    Show version information.

  - \<object\>|\<dir\> ...  
    An optional list of CSDB objects to list or directories to search
    for CSDB objects in. If none are specified, CSDB objects in the
    current directory are listed by default.

In addition, the following options allow configuration of the XML
parser:

  - \--dtdload  
    Load the external DTD.

  - \--huge  
    Remove any internal arbitrary parser limits.

  - \--net  
    Allow network access to load external DTD and entities.

  - \--noent  
    Resolve entities.

  - \--parser-errors  
    Emit errors from parser.

  - \--parser-warnings  
    Emit warnings from parser.

  - \--xinclude  
    Do XInclude processing.

  - \--xml-catalog \<file\>  
    Use an XML catalog when resolving entities. Multiple catalogs may be
    loaded by specifying this option multiple times.

# EXAMPLE

    $ s1kd-ls
    DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-001-01.JPG
    ICN-12345-00001-002-01.JPG
    PMC-EX-12345-00001-00_000-01_EN-CA.XML
    
    $ s1kd-ls -l
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-002-01.JPG
    PMC-EX-12345-00001-00_000-01_EN-CA.XML
    
    $ s1kd-ls -o
    DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
    ICN-12345-00001-001-01.JPG
    
    $ s1kd-ls -D | s1kd-metadata -lt -ntechName -ninfoName -nissueDate
    Example A    Description    2018-03-20
    Example A    Description    2018-03-29
    Example B    Description    2018-03-29
    
    $ s1kd-ls -Dl -e 'stat --printf="%n %Y\n" {}'
    DMC-EX-A-00-00-00-00A-040A-D_000-02_EN-CA.XML 1553738720
    DMC-EX-B-00-00-00-00A-040A-D_000-01_EN-CA.XML 1553738751
