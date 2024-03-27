# NAME

s1kd-newcom - Create a new S1000D comment.

# SYNOPSIS

    s1kd-newcom [options]

# DESCRIPTION

The *s1kd-newcom* tool creates a new S1000D comment with the code and
metadata specified.

# OPTIONS

  - \-\#, --code \<code\>  
    The code of the comment, in the form of
    MODELIDENTCODE-SENDERIDENT-YEAR-SEQ-TYPE.

  - \-$, --issue \<issue\>  
    Specify which issue of S1000D to use. Currently supported issues
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
    Save the new comment to \<path\>. If \<path\> is an existing
    directory, the comment will be created in it instead of the current
    directory. Otherwise, the comment will be saved as the filename
    \<path\> instead of being automatically named.

  - \-%, --templates \<dir\>  
    Use the XML template in the specified directory instead of the
    built-in template. The template must be named `comment.xml` inside
    \<dir\> and must conform to the default S1000D issue (5.0).

  - \-\~, --dump-templates \<dir\>  
    Dump the built-in XML template to the specified directory.

  - \-b, --brex \<BREX\>  
    BREX data module code.

  - \-C, --country \<country\>  
    The country ISO code of the new comment.

  - \-c, --security \<sec\>  
    The security classification of the new comment.

  - \-d, --defaults \<file\>  
    Specify the `.defaults` file name.

  - \-f, --overwrite  
    Overwrite existing file.

  - \-h, -?, --help  
    Show help/usage message.

  - \-I, --date \<date\>  
    The issue date of the new comment in the form of YYYY-MM-DD.

  - \-L, --language \<lang\>  
    The language ISO code of the new comment.

  - \-m, --remarks \<remarks\>  
    Set the remarks for the new comment.

  - \-o, --origname \<orig\>  
    The enterprise name of the originator of the comment.

  - \-P, --priority \<code\>  
    The priority code of the new comment.

  - \-p, --prompt  
    Prompt the user for values left unspecified.

  - \-q, --quiet  
    Do not report an error when the file already exists.

  - \-r, --response \<type\>  
    The response type of the new comment.

  - \-t, --title \<title\>  
    The title of the new comment.

  - \-v, --verbose  
    Print the file name of the newly created comment.

  - \-z, --issue-type \<type\>  
    The issue type of the new comment.

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

    $ s1kd-newcom -# EX-12345-2018-00001-Q
