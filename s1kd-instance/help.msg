Usage: s1kd-instance [-s <source>] [-e <ext>|-E] [-c <dmc>] [-l <lang>]
                     [-n <iss>] [-I <date>] [-o <file>|-O <dir>]
                     [-t <techName>] [-i <infoName>] [-a|-A] [-Y <text>]
                     [-C <comment>] [-g [-G <CODE>/<NAME>]]
                     [-R <CIR> ...] [-S] [-v] [<applic>...]

Options:
  -A            Remove unused applicability annotations and simplify statements.
  -a            Remove unused applicability annotations but not statements.
  -C <comment>  Add an XML comment to the top of the data module instance.
  -c <DMC>      The new data module code of the data module instance.
  -E            Remove extension from instance.
  -e <ext>      Specify an extension on the data module code (DME).
  -f            Overwrite file with same name as automatic output, if it exists.
  -G            Use custom NCAGE/name for originator.
  -g            Set originator of the instance to identify this tool.
  -h -?         Show this help/usage message.
  -I <date>     Set the issue date of the instance.
  -i <infoName> Give the data module instance a different infoName.
  -L            Source (-s/stdin) is a list of files instead of a DM.
  -l <lang>     Specify the language of the data module instance.
  -N            Omit issue/inwork numbers from automatic filename.
  -n <iss>      Set the issue and inwork numbers of the instance.
  -O <dir>      Output data module instance in dir, automatically named.
  -o <file>     Output data module instance to file instead of stdout.
  -P <PCT>      PCT file to read products from
  -p <id>       ID of product in specified PCT to read applic defs from.
  -R <CIR>      Resolve externalized items using the given CIR.
  -r <XSL>      Use custom XSLT to resolve CIR references.
  -S            Do not include <sourceDmIdent> or <repositorySourceDmIdent>.
  -s <source>   The source data module (default is stdin)
  -t <techName> Give the data module instance a different techName/pmTitle.
  -u <sec>      Set the security classification of the instance.
  -v            Print the file name of the instance when -O is used.
  -w            Check the applic of the whole DM.
  -x <CIR>      Dump default XSLT for resolving CIR references.
  -Y <text>     Set applic for DM with text as the display text.
  -y            Set applic for DM based on the user-defined defs.
  <applic>...   Any number of applic defs in the form of <ident>:<type>=<value>
