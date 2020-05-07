using System;
using S1kdTools;

/* Applicability filtering */

public class Test
{
	public static void Main(string[] args)
	{
		CsdbObject dm = new CsdbObject("test.xml");
		Applicability app = new Applicability();

		app.Assign("version", "prodattr", "A");

		Console.WriteLine(dm.Filter(app, FilterMode.Default).XmlDocument.OuterXml);
		Console.WriteLine(dm.Filter(app, FilterMode.Reduce).XmlDocument.OuterXml);
	}
}
