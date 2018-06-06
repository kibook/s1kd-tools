Usage: s1kd-instance [-e <ext>|-E] [-c <dmc>] [-l <lang>]
                     [-n <iss>] [-I <date>] [-o <file>|-O <dir>]
                     [-t <techName>] [-i <infoName>] [-a|-A] [-Y <text>]
                     [-C <comment>] [-X <path>] [-g [-G <CODE>/<NAME>]]
                     [-R <CIR> ...] [-S] [-v] [-s <applic> ...]
		     [<object>...]

Options:
  -A            Simplify and remove unused applicability annotations.
  -a            Only remove unused applicability annotation references.
  -C <comment>  Add an XML comment to the top of the instance.
  -c <DMC>      The new code of the instance.
  -E            Remove extension from instance.
  -e <ext>      Specify an extension on the instance code (DME/PME).
  -F            Flatten alts elements.
  -f            When using -O, overwrite file with same name if it exists.
  -G            Use custom NCAGE/name for originator.
  -g            Set originator of the instance to identify this tool.
  -h -?         Show this help/usage message.
  -I <date>     Set the issue date of the instance.
  -i <infoName> Give the data module instance a different infoName.
  -L            Source (-s/stdin) is a list of files instead of a DM.
  -l <lang>     Specify the language of the instance.
  -N            Omit issue/inwork numbers from automatic filename.
  -n <iss>      Set the issue and inwork numbers of the instance.
  -O <dir>      Output instance in dir, automatically named.
  -o <file>     Output instance to file instead of stdout.
  -P <PCT>      PCT file to read products from
  -p <id>       ID of product in specified PCT to read applic defs from.
  -R <CIR>      Resolve externalized items using the given CIR.
  -r <XSL>      Use custom XSLT to resolve CIR references.
  -S            Do not include <sourceDmIdent> or <repositorySourceDmIdent>.
  -s <applic>   An assign in the form of <ident>:<type>=<value>
  -t <techName> Give the instance a different techName/pmTitle.
  -u <sec>      Set the security classification of the instance.
  -v            Print the file name of the instance when -O is used.
  -w            Check the applic of the whole DM.
  -X            XPath where the -C comment will be inserted.
  -x <CIR>      Dump default XSLT for resolving CIR references.
  -Y <text>     Set applic for DM with text as the display text.
  -y            Set applic for DM based on the user-defined defs.
  --version     Show version information.
  object...     Source CSDB object(s)
