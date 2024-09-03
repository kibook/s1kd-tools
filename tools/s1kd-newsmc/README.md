# NAME

s1kd-newsmc - Create new S1000D SCORM content package

# SYNOPSIS

    s1kd-newsmc [options] [<DM>...]

# DESCRIPTION

The *s1kd-newsmc* tool creates a new S1000D SCORM content package with
the SCORM content package code and other metadata specified.

# OPTIONS

  - \-\#, --code \<SMC\>  
    The SCORM content package code of the new SCORM content package.

  - \-$, --issue \<issue\>  
    Specify which issue of S1000D to use. Currently supported issues
    are:
    
      - 6 (default)
    
      - 5.0
    
      - 4.2
    
      - 4.1

  - \-@, --out \<path\>  
    Save the new SCORM content package to \<path\>. If \<path\> is an
    existing directory, the SCORM content package will be created in it
    instead of the current directory. Otherwise, the SCORM content
    package will be saved as the filename \<path\> instead of being
    automatically named.

  - \-%, --templates \<dir\>  
    Use the XML template in \<dir\> instead of the built-in template.
    The template must be named `scormcontentpackage.xml` in \<dir\> and
    must conform to the default S1000D issue (6).

  - \-\~, --dump-templates \<dir\>  
    Dump the built-in XML template to the specified directory.

  - \-a, --act \<ACT\>  
    ACT data module code.

  - \-b, --brex \<BREX\>  
    BREX data module code.

  - \-C, --country \<country\>  
    The country ISO code of the new SCORM content package.

  - \-c, --security \<sec\>  
    The security classification of the new SCORM content package.

  - \-D, --include-date  
    Include issue date in referenced data modules.

  - \-d, --defaults \<file\>  
    Specify the `.defaults` file name.

  - \-f, --overwrite  
    Overwrite existing file.

  - \-h, -?, --help  
    Show help/usage message.

  - \-I, --date \<date\>  
    The issue date of the new SCORM content package in the form of
    YYYY-MM-DD.

  - \-i, --include-issue  
    Include issue information in referenced data modules.

  - \-k, --skill \<skill\>  
    The skill level code of the new SCORM content package.

  - \-L, --language \<language\>  
    The language ISO code of the new SCORM content package.

  - \-l, --include-lang  
    Include language information in referenced data modules.

  - \-m, --remarks \<remarks\>  
    Set remarks for the new SCORM content package.

  - \-n, --issno \<issue\>  
    The issue number of the new SCORM content package.

  - \-p, --prompt  
    Prompt the user for any values left unspecified.

  - \-q, --quiet  
    Do not report an error when the file already exists.

  - \-R, --rpccode \<CAGE\>  
    The CAGE code of the responsible partner company.

  - \-r, --rpcname \<RPC\>  
    The responsible partner company enterprise name of the new SCORM
    content package.

  - \-T, --include-title  
    Include titles in referenced data modules.

  - \-t, --title \<title\>  
    The title of the new SCORM content package.

  - \-v, --verbose  
    Print the file name of the newly created SCORM content package.

  - \-w, --inwork \<inwork\>  
    The inwork number of the new SCORM content package.

  - \-z, --issue-type \<type\>  
    The issue type of the new SCORM content package.

  - \--version  
    Show version information.

  - \<DM\>...  
    Any number of data modules to automatically reference in the new
    SCORM content package's content.

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

    $ s1kd-newsmc -# EX-12345-00001-00
