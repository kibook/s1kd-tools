# NAME

s1kd-validate - Validate S1000D CSDB objects against their schemas

# SYNOPSIS

    s1kd-validate [-s <path>] [-X <URI>] [-F|-f] [-o|-x] [-elqTv^h?]
                  [<object>...]

# DESCRIPTION

The *s1kd-validate* tool validates S1000D CSDB objects, checking whether
they are valid XML files and if they are valid against their own S1000D
schemas.

# OPTIONS

  - \-e, --ignore-empty  
    Ignore validation for empty or non-XML documents.

  - \-F, --valid-filenames  
    List valid files.

  - \-f, --filenames  
    List invalid files.

  - \-h, -?, --help  
    Show help/usage message.

  - \-l, --list  
    Treat input as a list of object names to validate, rather than an
    object itself.

  - \-o, --output-valid  
    Output valid CSDB objects to stdout.

  - \-q, --quiet  
    Quiet mode. The tool will not output anything to stdout or stderr.
    Success/failure will only be indicated through the exit status.

  - \-s, --schema \<path\>  
    Validate the objects against the specified schema, rather than the
    one that they reference.

  - \-T, --summary  
    Print a summary of the validation after it completes, including
    statistics on the number of documents that passed/failed.

  - \-v, --verbose  
    Verbose mode. Success/failure will be explicitly reported on top of
    any errors.

  - \-X, --exclude \<URI\>  
    Exclude an XML namespace from the validation. Elements in the
    namespace specified by \<URI\> are ignored.

  - \-x, --xml  
    Output an XML report.

  - \-^, --remove-deleted  
    Validate with elements that have a change type of "delete" removed.

  - \--version  
    Show version information.

  - \<object\>...  
    Any number of CSDB objects to validate. If none are specified, input
    is read from stdin.

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

## XML catalogs

XML catalogs allow redirecting the canonical URIs of XML schemas and
other external resources to local files, so as to avoid the unnecessary
overhead of downloading those static resources over the Internet.

Below is an example of a catalog file which maps the S1000D schemas to a
local directory:

    <catalog xmlns="urn:oasis:names:tc:entity:xmlns:xml:catalog">
    <rewriteURI
    uriStartString="http://www.s1000d.org"
    rewritePrefix="/usr/share/s1kd/schemas"/>
    </catalog>

This can be placed in a catalog file automatically loaded by libxml2
(e.g., `/etc/xml/catalog`) or saved to a file which is then specified in
an environment variable used by libxml2 (e.g., `XML_CATALOG_FILES`):

    $ XML_CATALOG_FILES=catalog.xml s1kd-validate <DMs...>

Alternatively, the --xml-catalog option may be used:

    $ s1kd-validate --xml-catalog=catalog.xml <DMs>...

# EXIT STATUS

  - 0  
    No errors.

  - 1  
    Some CSDB objects are not well-formed or valid.

  - 2  
    The number of schemas cached exceeded the available memory.

  - 3  
    A specified schema could not be read.

# EXAMPLE

    $ s1kd-validate DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
