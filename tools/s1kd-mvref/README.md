# NAME

s1kd-mvref - Change one reference in to another in S1000D CSDB objects

# SYNOPSIS

    s1kd-mvref [-d <dir>] [-s <source>] [-t <target>] [-cflqvh?]
               [<object>...]

# DESCRIPTION

The *s1kd-mvref* tool changes all references to one object (the source
object) into references to another object (the target object) in a
specified set of objects.

# OPTIONS

  - \-c, --content  
    Only move references within the content section of objects.

  - \-d, --dir \<dir\>  
    Move references in all objects in the specified directory.

  - \-f, --overwrite  
    Overwrite updated input objects.

  - \-h, -?, --help  
    Show help/usage message

  - \-l, --list  
    Treat input as a list of data module filenames, rather than a data
    module itself.

  - \-q, --quiet  
    Quiet mode. Errors are not printed.

  - \-s, --source \<source\>  
    The source object.

  - \-t, --target \<target\>  
    Change all references to the source object specified with -s into
    references that point to \<target\>.

  - \-v, --verbose  
    Verbose output.

  - \--version  
    Show version information.

  - \<object\>...  
    Objects to move references in.

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

    $ s1kd-mvref -f -s <old DM> -t <new DM> DMC-*.XML
