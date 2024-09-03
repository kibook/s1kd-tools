# NAME

s1kd-metadata - View and edit S1000D CSDB object metadata

# SYNOPSIS

    s1kd-metadata [options] [<object>...]

# DESCRIPTION

The *s1kd-metadata* tool provides a simple way to fetch and change
metadata on S1000D CSDB objects.

# OPTIONS

  - \-0, --null  
    Print a null-delimited list of values of the pieces of metadata
    specified with -n, or all available metadata if -n is not specified.

  - \-c, --set \<file\>  
    Use \<file\> to edit metadata files. \<file\> consists of lines
    starting with a metadata name, followed by whitespace, followed by
    the new value for the metadata (the program uses this same format
    when outputting all metadata if no \<name\> is specified).

  - \-d, --date-format \<fmt\>  
    The format to use when printing dates, such as the "issueDate" or
    "modified" metadata. \<fmt\> should conform to the format used by
    strftime. The default is "%Y-%m-%d".

  - \-E, --editable  
    When showing all metadata, only list editable items. This is useful
    when creating a file for use with the -c option.

  - \-e, --exec \<cmd\>  
    Execute a command for each CSDB object. The string "{}" is replaced
    by the current CSDB object file name everywhere it occurs in the
    arguments to the command.

  - \-F, --format \<fmt\>  
    Print a formatted line for each CSDB object. Metadata names
    surrounded with % (e.g. %issueDate%) will be substituted by the
    value read from the object.

  - \-f, --overwrite  
    When editing metadata, overwrite the object. The default is to
    output the modified object to stdout.

  - \-H, --info  
    Lists all available metadata with a short description of each.
    Specify specific metadata to describe with the -n option.

  - \-h, -?, --help  
    Show help/usage message.

  - \-l, --list  
    Treat input as a list of object filenames to read or edit metadata
    on, rather than an object itself.

  - \-m, --matches \<regex\>  
    Used after a -w or -W option, this specifies a regular expression to
    match the value of the given metadata against, instead of a literal
    value (-v).

  - \-n, --name \<name\>  
    The name of the piece of metadata to fetch. This option can be
    specified multiple times to fetch multiple pieces of metadata. If -n
    is not specified, all available metadata names are printed with
    their values. This output can be sent to a text file, edited, and
    then specified with the -c option as a means of editing metadata in
    any text editor.

  - \-q, --quiet  
    Quiet mode. Non-fatal errors such as a missing piece of optional
    metadata in an object will not be printed to stderr.

  - \-T, --raw  
    Do not format columns in output.

  - \-t, --tab  
    Print a tab-delimited list of values of the pieces of metadata
    specified with -n, or all available metadata if -n is not specified.

  - \-v, --value \<value\>  
    When following a -n option, this specifies the new value for that
    piece of metadata.
    
    When following a -w or -W option, this specifies the value to
    compare that piece of metadata to.
    
    Each -n, -w, or -W can be followed by -v to edit or define
    conditions on multiple pieces of metadata.

  - \-W, --where-not \<name\>  
    Show or edit metadata only on objects where the value of \<name\> is
    not equal to the value specified in the following -v option. If no
    -v option follows, this will show objects which do not have metadata
    \<name\> of any value.

  - \-w, --where \<name\>  
    Show or edit metadata only on objects where the value of \<name\> is
    equal to the value specified in the following -v option. If no -v
    option follows, this will show objects which have metadata \<name\>
    with any value.

  - \--version  
    Show version information.

  - \<object\>...  
    The object(s) to show/edit metadata on. The default is to read from
    stdin.

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

    $ ls
    DMC-S1KDTOOLS-A-09-00-00-00A-040A-D_EN-CA.XML
    DMC-S1KDTOOLS-A-0Q-00-00-00A-040A-D_EN-CA.XML
    
    $ DMOD=DMC-S1KDTOOLS-A-09-00-00-00A-040A-D_EN-CA.XML
    $ s1kd-metadata $DMOD
    issueDate                      2017-08-14
    techName                       s1kd-metadata(1) | s1kd-tools
    responsiblePartnerCompany      khzae.net
    originator                     khzae.net
    securityClassification         01
    schema                         descript
    schemaUrl                      http://www.s1000d.org/S1000D_6/xml_
    schema_flat/descript.xsd
    type                           dmodule
    applic                         All
    brex                           S1000D-F-04-10-0301-00A-022A-D
    issueType                      new
    languageIsoCode                en
    countryIsoCode                 CA
    issueNumber                    001
    inWork                         00
    dmCode                         S1KDTOOLS-A-09-00-00-00A-040A-D
    
    $ s1kd-metadata -n techName -v "New title" $DMOD
    $ s1kd-metadata -n techName $DMOD
    New title
    
    $ s1kd-metadata -n techName DMC-*.XML
    New title
    s1kd-aspp(1) | s1kd-tools
    
    $ s1kd-metadata -F "%techName% (%issueDate%) %issueType%" DMC-*.XML
    New title (2017-08-14) new
    s1kd-aspp(1) | s1kd-tools (2018-03-28) changed
    
    $ s1kd-metadata -F "%techName%" -w subSubSystemCode -v Q DMC-*.XML
    s1kd-aspp(1) | s1kd-tools
    
    $ s1kd-metadata -n path -w subSystemCode -v Q
    DMC-S1KDTOOLS-A-0Q-00-00-00A-040A-D_EN-CA.XML
    
    $ s1kd-metadata -n path -W subSystemCode -v Q
    DMC-S1KDTOOLS-A-09-00-00-00A-040A-D_EN-CA.XML
    
    $ s1kd-metadata -n path -w subSystemCode -m [0-9]
    DMC-S1KDTOOLS-A-09-00-00-00A-040A-D_EN-CA.XML
