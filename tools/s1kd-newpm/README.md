# NAME

s1kd-newpm - Create new S1000D publication module.

# SYNOPSIS

    s1kd-newpm [options] [<DM>...]

# DESCRIPTION

The *s1kd-newpm* tool creates a new S1000D publication module with the
publication module code and other metadata specified.

# OPTIONS

  - \-\#, --code \<PMC\>  
    The publication module code of the new publication module.

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
    Save the new publication module to \<path\>. If \<path\> is an
    existing directory, the publication module will be created in it
    instead of the current directory. Otherwise, the publication module
    will be saved as the filename \<path\> instead of being
    automatically named.

  - \-%, --templates \<dir\>  
    Use the XML template in \<dir\> instead of the built-in template.
    The template must be named `pm.xml` in \<dir\> and must conform to
    the default S1000D issue (5.0).

  - \-\~, --dump-templates \<dir\>  
    Dump the built-in XML template to the specified directory.

  - \-a, --act \<ACT\>  
    ACT data module code.

  - \-b, --brex \<BREX\>  
    BREX data module code.

  - \-C, --country \<country\>  
    The country ISO code of the new publication module.

  - \-c, --security \<sec\>  
    The security classification of the new publication module.

  - \-D, --include-date  
    Include issue date in referenced data modules.

  - \-d, --defaults \<file\>  
    Specify the `.defaults` file name.

  - \-f, --overwrite  
    Overwrite existing file.

  - \-h, -?, --help  
    Show help/usage message.

  - \-I, --date \<date\>  
    The issue date of the new publication module in the form of
    YYYY-MM-DD.

  - \-i, --include-issue  
    Include issue information in referenced data modules.

  - \-L, --language \<language\>  
    The language ISO code of the new publication module.

  - \-l, --include-lang  
    Include language information in referenced data modules.

  - \-m, --remarks \<remarks\>  
    Set remarks for the new publication module.

  - \-n, --issno \<issue\>  
    The issue number of the new publication module.

  - \-p, --prompt  
    Prompt the user for any values left unspecified.

  - \-q, --quiet  
    Do not report an error when the file already exists.

  - \-R, --rpccode \<CAGE\>  
    The CAGE code of the responsible partner company.

  - \-r, --rpcname \<RPC\>  
    The responsible partner company enterprise name of the new
    publication module.

  - \-s, --short-title \<title\>  
    The short title of the new publication module.

  - \-T, --include-title  
    Include titles in referenced data modules.

  - \-t, --title \<title\>  
    The title of the new publication module.

  - \-v, --verbose  
    Print the file name of the newly created publication module.

  - \-w, --inwork \<inwork\>  
    The inwork number of the new publication module.

  - \-z, --issue-type \<type\>  
    The issue type of the new publication module.

  - \--version  
    Show version information.

  - \<DM\>...  
    Any number of data modules to automatically reference in the new
    publication module's content.

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

    $ s1kd-newpm -# EX-12345-00001-00
