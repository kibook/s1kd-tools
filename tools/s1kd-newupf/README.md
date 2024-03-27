# NAME

s1kd-newupf - Create a new data update file

# SYNOPSIS

    s1kd-newupf [options] <SOURCE> <TARGET>

# DESCRIPTION

The *s1kd-newupf* tool creates a new S1000D data update file for two
specified issues of a CIR data module. Changes to items between the
source and target issues of the CIR are recorded in the resulting UPF,
along with update instructions.

# OPTIONS

  - \-$, --issue \<issue\>  
    Specify which issue of S1000D to use. Currently supported issues
    are:
    
      - 5.0 (default)
    
      - 4.2
    
      - 4.1

  - \-@, --out \<path\>  
    Save the new update file to \<path\>. If \<path\> is an existing
    directory, the update file will be created in it instead of the
    current directory. Otherwise, the update file will be saved as the
    filename \<path\> instead of being automatically named.

  - \-%, --templates \<dir\>  
    Use XML template in the specified directory instead of the built-in
    template. The template must be named `update.xml` in the directory
    \<dir\>, and must conform to the default S1000D issue of this tool
    (5.0).

  - \-\~, --dump-templates \<dir\>  
    Dump the built-in XML template to the specified directory.

  - \-d, --defaults \<file\>  
    Specify the `.defaults` file name.

  - \-f, --overwrite  
    Overwrite existing file.

  - \-h, -?, --help  
    Show help/usage message.

  - \-q, --quiet  
    Do not report an error when the file already exists.

  - \-v, --verbose  
    Print the file name of the newly created data update file.

  - \--version  
    Show version information.

  - \<SOURCE\>  
    The source (original) issue of the CIR data module.

  - \<TARGET\>  
    The target (updated) issue of the CIR data module.

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
used by all the s1kd-new\* tools.

# EXAMPLE

    $ s1kd-newupf \
        DMC-EX-A-00-00-00-00A-00GA-D_001-00_EN-CA.XML \
        DMC-EX-A-00-00-00-00A-00GA-D_002-00_EN-CA.XML
