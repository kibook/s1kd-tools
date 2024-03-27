# NAME

s1kd-refs - Manage references between CSDB objects

# SYNOPSIS

    s1kd-refs [-aBCcDEFfGHIiKLlmNnoPqRrSsTUuvwXxYZ^h?] [-b <SNS>]
              [-d <dir>] [-e <cmd>] [-J <ns=URL>] [-j <xpath>]
              [-k <pattern>] [-t <fmt>] [-3 <file>] [<object>...]

# DESCRIPTION

The *s1kd-refs* tool lists external references in CSDB objects,
optionally matching them to a filename in the CSDB directory hierarchy.

This allows you to:

  - obtain a list of dependencies for CSDB objects, such as ICNs, to
    ensure they are delivered together

  - check for references to objects which do not exist in the current
    CSDB

  - update reference metadata, such as titles, from the matched objects

# OPTIONS

  - \-a, --all  
    List all references, both matched and unmatched.

  - \-B, -C, -D, -E, -G, -H, -K, -L, -P, -S, -T, -Y, -Z  
    List references to IPDs, comments, data modules, external
    publications, ICNs, hotspots, CSNs, data management lists,
    publication modules, SCORM content packages, referred fragments,
    repository source DMs and source objects respectively. If none are
    specified, -BCDEGHKLPSTYZ is assumed.
    
    The following long options can also be used for each: --ipd, --com,
    --dm, --epr, --icn, --hotspot, --csn, --dml, --pm, --smc,
    --fragment, --repository, --source.

  - \-b, --ipd-sns \<SNS\>  
    Specify the SNS for non-chapterized IPD data modules, in the form of
    SYSTEM-SUBSYSTEM-ASSY (for example, "ZD-00-35"). This code is used
    to resolve non-chapterized CSN references.
    
    If "-" is given for \<SNS\>, then the SNS will be derived from
    current data module.

  - \-c, --content  
    List references in the `content` section of a CSDB object only.

  - \-d, --dir \<dir\>  
    Directory to search for matches to references in. By default, the
    current directory is used.

  - \-e, --exec \<cmd\>  
    Execute a command for each referenced CSDB object matched. The
    string "{}" is replaced by the current CSDB object file name
    everywhere it occurs in the arguments to the command.

  - \-F, --overwrite  
    When using the -U or -X options, overwrite the input objects that
    have been updated or tagged.

  - \-f, --filename  
    Include the filename of the source object where each reference was
    found in the output.

  - \-h, -?, --help  
    Show help/usage message.

  - \-I, --update-issue  
    Update the issue number, issue date, language, and title of
    references to that of the latest matched object. This option implies
    the -U and -i options.

  - \-i, --ignore-issue  
    Ignore issue info when matching. This will always match the latest
    issue of an object found, regardless of the issue specified in the
    reference.

  - \-J \<ns=URL\>  
    Registers an XML namespace prefix, which can then be used in the
    hotspot XPath expression (-j). Multiple namespaces can be registered
    by specifying this option multiple times.

  - \-j \<xpath\>  
    Specify a custom XPath expression to use when matching hotspots (-H)
    in XML-based ICN formats.

  - \-k, --ipd-dcv \<pattern\>  
    Specify a pattern used to determine the disassembly code variant for
    IPD data modules when resolving CSN references.
    
    Within the pattern, the following characters have special meaning:
    
      - % - The figure number variant code.
    
      - ? - A wildcard that matches any single character.
    
    The default pattern is "%", which means the disassembly code variant
    is exactly the same as the figure number variant. Projects that use
    a 2- or 3-character disassembly code variant must specify a pattern
    of the appropriate length in order for their IPD DMs to be matched
    (for example, "%?" or "%??").

  - \-l, --list  
    Treat input (stdin or arguments) as lists of filenames of CSDB
    objects to list references in, rather than CSDB objects themselves.

  - \-m, --strict-match  
    Be more strict when matching codes of CSDB objects to filenames. By
    default, the name of a file (minus the extension) only needs to
    start with the code to be matched. When this option is specified,
    the name must match the code exactly.
    
    For example, the code "ABC" will normally match either of the files
    "ABC.PDF" or "ABC\_1.PDF", but when strict matching is enabled, it
    will only match the former.

  - \-N, --omit-issue  
    Assume filenames of referenced CSDB objects omit the issue info,
    i.e. they were created with the -N option to the s1kd-new\* tools.

  - \-n, --lineno  
    Include the filename of the source object where each reference was
    found, and display the line number where the reference occurs in the
    source file after its filename.

  - \-o, --output-valid  
    Output valid CSDB objects to stdout.

  - \-q, --quiet  
    Quiet mode. Errors are not printed.

  - \-R, --recursively  
    List references in matched objects recursively.

  - \-r, --recursive  
    Search for matches to references in directories recursively.

  - \-s, --include-src  
    Include the source object as a reference. This is helpful when the
    output of this tool is used to apply some operation to a source
    object and all its dependencies together.

  - \-t, --format \<fmt\>  
    Specify a custom format for printed references. \<fmt\> is a format
    string, where the following variables can be given:
    
      - %file% - The filename of the referenced object (nothing is
        printed if no file is matched).
    
      - %line% - The line number where the reference occurs in the
        source.
    
      - %ref% - The reference. May be a code (if no file is matched), a
        file name (for objects where a file is matched) or a file name +
        fragment name.
    
      - %src% - The source of the reference.
    
      - %xpath% - The XPath denoting where the reference occurs in the
        source.
    
    For example, `-t '%src% (%line%): %ref%'` is equivalent to the -n
    option.

  - \-U, --update  
    Update the title of matched references from the corresponding
    object.

  - \-u, --unmatched  
    Show only unmatched reference errors, or unmatched codes if combined
    with the -a option.

  - \-v, --verbose  
    Verbose output. Specify multiple times to increase the verbosity.

  - \-w, --where-used  
    Instead of listing references contained within specified objects,
    list places within other objects where the specified objects are
    referenced.
    
    In this case, \<object\> may also be a code (with the appropriate
    prefix) instead of an actual file. For example: `s1kd-refs -w
    DMC-TEST-A-00-00-00-00A-040A-D`

  - \-X, --tag-unmatched  
    Tag unmatched references with the processing instruction
    `<?unmatched?>`.

  - \-x, --xml  
    Output a detailed XML report instead of plain text messages.

  - \-3, --externalpubs \<file\>  
    Use a custom `.externalpubs` file.

  - \-^, --remove-deleted  
    List references with elements that have a change type of "delete"
    removed.

  - \--version  
    Show version information.

  - \<object\>...  
    CSDB object(s) to list references in. If none are specified, the
    tool will read from stdin.

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

## `.externalpubs` file

The `.externalpubs` file contains definitions of external publication
references. This can be used to update external publication references
in CSDB objects with -U.

By default, the tool will search the current directory and parent
directories for a file named .externalpubs, but any file can be
specified by using the -e option.

Example of a `.externalpubs` file:

    <externalPubs>
    <externalPubRef>
    <externalPubRefIdent>
    <externalPubCode>ABC</externalPubCode>
    <externalPubTitle>ABC Manual</externalPubTitle>
    </externalPubRefIdent>
    </externalPubRef>
    </externalPubs>

External publication references will be updated whether they are matched
to a file or not.

## Hotspot matching (-H)

Hotspots can be matched in XML-based ICN formats, such as SVG or X3D. By
default, matching is based on the APS ID of the hotspot and the
following attributes:

  - SVG  
    `@id`

  - X3D  
    `@DEF`

If hotspots are identified in a different way in a project's ICNs, a
custom XPath expression can be specified with the -j option. In this
XPath expression, the variable `$id` represents the hotspot APS ID:

    $ s1kd-refs -H -j "//*[@attr = $id]" <DM>

# EXIT STATUS

  - 0  
    No errors, all references were matched.

  - 1  
    Some references were unmatched.

  - 2  
    The number of objects found in a recursive check (-R) exceeded the
    available memory.

  - 3  
    stdin did not contain valid XML and not in list mode (-l).

  - 4  
    The non-chapterized SNS specified (-b) is not valid.

# EXAMPLES

## General

    $ s1kd-refs DMC-EX-A-00-00-00-00A-040A-D_000-01_EN-CA.XML
      DMC-EX-A-00-00-00-00A-022A-D_001-00_EN-CA.XML
      DMC-EX-A-01-00-00-00A-040A-D_000-01_EN-CA.XML
      ICN-12345-00001-001-01.JPG

## CSN references

These examples are based on the following CSN reference:

    <catalogSeqNumberRef figureNumber="01" item="004"/>

in the following data module:

    DM=DMC-EX-A-00-00-00-00AA-100A-D_001-00_EN-CA.XML

Because the CSN reference is not chapterized, it cannot be matched to an
IPD DM without more information:

    $ s1kd-refs -K $DM
    Unmatched reference: Fig 01 Item 004

The SNS for non-chapterized IPDs can be specified with -b. In this case,
the project uses the SNS "ZD-00-35" for their IPDs:

    $ s1kd-refs -K -b ZD-00-35 $DM
    Unmatched reference: DMC-EX-A-ZD-00-35-010-941A-D Item 004

This project uses a 2-character disassembly code variant, so the figure
number variant is not sufficient to resolve the DMC of the referenced
IPD data module. The -k option can be used in this case to specify the
pattern for the disassembly code variant of IPDs. Since the second
character of the disassembly code variant of all IPD DMs in this project
is A, the pattern "%A" can be used:

    $ s1kd-refs -K -b ZD-00-35 -k %A $DM
    DMC-EX-A-ZD-00-35-010A-941A-D_001-00_EN-CA.XML Item 004
