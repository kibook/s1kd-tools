using System;
using System.Xml;
using S1kdTools;

/* BREX checking */

public class Test
{
	public static void PrintResults(XmlDocument report)
	{
		if (report.DocumentElement.SelectSingleNode("//brex/error") != null) {
			Console.WriteLine("There were some BREX errors");
		} else {
			Console.WriteLine("There were no BREX errors");
		}

		Console.WriteLine(report.OuterXml);
	}

	public static void Main(string[] args)
	{
		CsdbObject dm = new CsdbObject("test.xml");
		BrexCheckOptions options = new BrexCheckOptions();
		XmlDocument report;

		options.CheckValues = true;
		options.CheckSns = true;
		options.StrictSns = true;
		options.CheckNotations = true;

		report = dm.CheckAgainstDefaultBREX();
		PrintResults(report);

		report = dm.CheckAgainstDefaultBREX(options);
		PrintResults(report);

		report = dm.CheckAgainstBREX("brex.xml");
		PrintResults(report);

		report = dm.CheckAgainstBREX("brex.xml", options);
		PrintResults(report);
	}
}
