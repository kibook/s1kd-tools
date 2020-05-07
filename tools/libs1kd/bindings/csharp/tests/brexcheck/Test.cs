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
	}

	public static void Main(string[] args)
	{
		CsdbObject dm = new CsdbObject("test.xml");

		XmlDocument report = dm.CheckAgainstDefaultBREX();
		PrintResults(report);

		report = dm.CheckAgainstBREX("brex.xml");
		PrintResults(report);
	}
}
