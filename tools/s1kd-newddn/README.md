# NAME

s1kd-newddn - Create an S1000D Data Dispatch Note (DDN).

# SYNOPSIS

    s1kd-newddn [options] <files>...

# DESCRIPTION

The *s1kd-newddn* tool creates a new S1000D data dispatch note with the
code, metadata, and list of files specified.

# OPTIONS

  - \-\#, --code \<code\>  
    The code of the new data dispatch note, in the form of
    MODELIDENTCODE-SENDER-RECEIVER-YEAR-SEQUENCE.

  - \-$, --issue \<issue\>  
    Specifiy which issue of S1000D to use. Currently supported issues
    are:
    
      - 5.0 (default)
    
      - 4.2
    
      - 4.1
    
      - 4.0
    
      - 3.0
    
      - 2.3
    
      - 2.2
    
      - 2.1
    
      - 2.0

  - \-@, --out \<path\>  
    Save the new DDN to \<path\>. If \<path\> is an existing directory,
    the DDN will be created in it instead of the current directory.
    Otherwise, the DDN will be saved as the filename \<path\> instead of
    being automatically named.

  - \-%, --templates \<dir\>  
    Use the XML template in the specified directory instead of the
    built-in template. The template must be named `ddn.xml` inside
    \<dir\> and must conform to the default S1000D issue (5.0).

  - \-\~, --dump-templates \<dir\>  
    Dump the built-in XML template to the specified directory.

  - \-a, --authorization \<auth\>  
    Specify the authorization.

  - \-b, --brex \<BREX\>  
    BREX data module code.

  - \-d, --defaults \<file\>  
    Specify the `.defaults` file name.

  - \-f, --overwrite  
    Overwrite existing file.

  - \-h, -?, --help  
    Show help/usage message.

  - \-I, --date \<date\>  
    The issue date of the new DDN in the form of YYYY-MM-DD.

  - \-m, --remarks \<remarks\>  
    Set the remarks for the new data dispatch note.

  - \-N, --receiver-country \<country\>  
    The receiver's country.

  - \-n, --sender-country \<country\>  
    The sender's country.

  - \-o, --sender \<name\>  
    The enterprise name of the sender.

  - \-p, --prompt  
    Prompt the user for values left unspecified.

  - \-q, --quiet  
    Do not report an error when the file already exists.

  - \-r, --receiver \<name\>  
    The enterprise name of the receiver.

  - \-T, --receiver-city \<city\>  
    The receiver's city.

  - \-t, --sender-city \<city\>  
    The sender's city.

  - \-v, --verbose  
    Print the file name of the newly created DDN.

  - \--version  
    Show version information.

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

## `.defaults` file

Refer to s1kd-newdm(1) for information on the `.defaults` file which is
used by all the s1kd-new\* commands.

# EXAMPLE

    $ s1kd-newddn -# EX-12345-54321-2018-00001
