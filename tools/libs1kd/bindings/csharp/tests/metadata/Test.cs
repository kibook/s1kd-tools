using System;
using S1kdTools;

/* Get/set metadata */

public class Test
{
	public static void Main(string[] args)
	{
		CsdbObject dm = new CsdbObject("test.xml");

		Console.WriteLine("DMC: " + dm.DmCode);

		Console.WriteLine("Issue date: " + dm.IssueDate);
		dm.IssueDate = "1970-01-01";
		Console.WriteLine("Issue date: " + dm.IssueDate);

		Console.WriteLine("Schema: " + dm.Schema);

		Console.WriteLine("S1000D Issue: " + dm.Issue);
		dm.Issue = "4.1";
		Console.WriteLine("S1000D Issue: " + dm.Issue);

		Console.WriteLine("Issue info: " + dm.IssueInfo);
	}
}
