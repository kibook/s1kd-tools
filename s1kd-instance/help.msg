Usage: s1kd-instance [-s <source>] [-c <dmc>] [-l <lang>] [-I <iss>]
                     [-o <file>|-O <dir>] [-t <techName>] [-i <infoName>]
		     [-a|-A] [-Y <text>] [-C <comment>] [-R <CIR> ...] [-S]
		     [<applic>...]

Options:
  -s <source>	The source data module (default is stdin)
  -e <ext>	Specify an extension on the data module code (DME).
  -c <DMC>	The new data module code of the data module instance.
  -l <lang>	Specify the language of the data module instance.
  -I <iss>      Set the issue and inwork numbers of the instance.
  -u <sec>      Set the security classification of the instance.
  -o <file>	Output data module instance to file instead of stdout.
  -O <dir>	Output data module instance in dir, automatically named.
  -f		Overwrite file with same name as automatic output, if it exists.
  -t <techName>	Give the data module instance a different techName.
  -i <infoName>	Give the data module instance a different infoName.
  -a		Remove unused applicability annotations but not statements.
  -A		Remove unused applicability annotations and simplify statements.
  -Y <text>	Set applic for DM with text as the display text.
  -S		Do not include <sourceDmIdent> or <repositorySourceDmIdent> in instance.
  -C <comment>	Add an XML comment to the top of the data module instance.
  -R <CIR>      Resolve externalized items using the given CIR.
  -w            Check the applic of the whole DM.
  -N            Omit issue/inwork numbers from automatic filename.
  <applic>...	Any number of applic defs in the form of <ident>:<type>=<value>
