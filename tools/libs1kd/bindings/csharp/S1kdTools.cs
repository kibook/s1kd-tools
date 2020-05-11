using System;
using System.Text;
using System.Xml;
using System.Runtime.InteropServices;

/// <summary>
/// C# interface for the s1kd-tools.
/// </summary>
namespace S1kdTools {
	/// <summary>
	/// Applicability definitions used for filtering.
	/// </summary>
	public class Applicability {
		unsafe private void *internalDefs;

		[DllImport("libs1kd")]
		unsafe private static extern void *s1kdNewApplicability();

		[DllImport("libs1kd")]
		unsafe private static extern void s1kdFreeApplicability(void *app);

		[DllImport("libs1kd")]
		unsafe private static extern void s1kdAssign(void *app, string id, string type, string val);

		unsafe private void InitInternalDefs()
		{
			internalDefs = s1kdNewApplicability();
		}

		unsafe private void FreeInternalDefs()
		{
			s1kdFreeApplicability(internalDefs);
		}

		unsafe private void AssignInternalDef(string id, string type, string val)
		{
			s1kdAssign(internalDefs, id, type, val);
		}

		unsafe public void *InternalDefs {
			get { return internalDefs; }
		}

		/// <summary>
		/// Creates a new set of applicability definitions.
		/// </summary>
		public Applicability()
		{
			InitInternalDefs();
		}

		~Applicability()
		{
			FreeInternalDefs();
		}

		/// <summary>
		/// Add an applicability definition to the set.
		/// </summary>
		public void Assign(string id, string type, string val)
		{
			AssignInternalDef(id, type, val);
		}
	}

	/// <summary>
	/// The mode used when filtering a CSDB object.
	/// </summary>
	public enum FilterMode
	{
		Default,  /// <value>The default filtering mode.</value>
		Reduce,   /// <value>Remove wholly resolved annotations.</value>
		Simplify, /// <value>Remove resolved parts of annotations.</value>
		Prune,    /// <value>Only remove false parts of annotations.</value>
	}

	/// <summary>
	/// BREX check options.
	/// </summary>
	public class BrexCheckOptions
	{
		internal int bits = 0;

		private enum Option
		{
			Values = 1
		}

		private bool GetOpt(Option opt)
		{
			return (bits & (int) opt) == (int) opt;
		}

		private void SetOpt(Option opt, bool val)
		{
			if (val) {
				bits |= (int) opt;
			} else {
				bits &= ~((int ) opt);
			}
		}

		/// <summary>
		/// Check object values.
		/// </summary>
		public bool CheckValues {
			get { return GetOpt(Option.Values); }
			set { SetOpt(Option.Values, value); }
		}
	}

	/// <summary>
	/// A CSDB object.
	/// </summary>
	public class CsdbObject
	{
		private XmlDocument doc;

		[DllImport("libs1kd")]
		unsafe private static extern int s1kdCheckDefaultBREX(string object_xml, int object_size, int options, out string report_xml, out int report_size);

		[DllImport("libs1kd")]
		unsafe private static extern int s1kdCheckBREX(string object_xml, int object_size, String brex_xml, int brex_size, int options, out string report_xml, out int report_size);

		[DllImport("libs1kd")]
		unsafe private static extern int s1kdFilter(string object_xml, int object_size, void *app, FilterMode mode, out string result_xml, out int result_size);

		[DllImport("libs1kd")]
		unsafe private static extern string s1kdGetMetadata(string object_xml, int object_size, string name);

		[DllImport("libs1kd")]
		unsafe private static extern int s1kdSetMetadata(string object_xml, int object_size, string name, string val, out string result_xml, out int result_size);

		unsafe private XmlDocument CheckAgainstBREXImpl(XmlDocument brex, BrexCheckOptions options)
		{
			string objectXml = doc.OuterXml;
			string brexXml = brex.OuterXml;
			string reportXml;
			int size;
			
			s1kdCheckBREX(objectXml, objectXml.Length, brexXml, brexXml.Length, options.bits, out reportXml, out size);

			XmlDocument report = new XmlDocument();
			report.LoadXml(reportXml);

			return report;
		}

		unsafe private XmlDocument CheckAgainstDefaultBREXImpl(BrexCheckOptions options)
		{
			string objectXml = doc.OuterXml;
			string reportXml;
			int size;

			s1kdCheckDefaultBREX(objectXml, objectXml.Length, options.bits, out reportXml, out size);

			XmlDocument report = new XmlDocument();
			report.LoadXml(reportXml);

			return report;
		}

		unsafe private CsdbObject FilterImpl(Applicability app, FilterMode mode)
		{
			string objectXml = doc.OuterXml;
			string resultXml;
			int size;

			int err = s1kdFilter(objectXml, objectXml.Length, app.InternalDefs, mode, out resultXml, out size);

			if (err != 0) {
				throw new Exception("Filtering failed (" + err + ")");
			}

			CsdbObject result = new CsdbObject();
			result.XmlDocument.LoadXml(resultXml);

			return result;
		}

		unsafe private string GetMetadataImpl(string name)
		{
			string objectXml = doc.OuterXml;
			
			return s1kdGetMetadata(objectXml, objectXml.Length, name);
		}

		unsafe private void SetMetadataImpl(string name, string val)
		{
			string objectXml = doc.OuterXml;
			string resultXml;
			int size;

			int err = s1kdSetMetadata(objectXml, objectXml.Length, name, val, out resultXml, out size);

			if (err != 0) {
				throw new Exception("Set metadata failed (" + err + ")");
			}

			doc.LoadXml(resultXml);
		}
	
		/// <summary>
		/// Internal XML document containing the XML tree for the CSDB object.
		/// </summary>
		public XmlDocument XmlDocument {
			get { return doc; }
		}

		/// <summary>
		/// Create a new, empty CSDB object.
		/// </summary>
		public CsdbObject()
		{
			doc = new XmlDocument();
		}

		/// <summary>
		/// Create a new CSDB object from an existing XML document.
		/// </summary>
		public CsdbObject(XmlDocument doc)
		{
			this.doc = doc;
		}

		/// <summary>
		/// Create a new CSDB object from an XML file.
		/// </summary>
		public CsdbObject(string path)
		{
			doc = new XmlDocument();
			doc.Load(path);
		}

		/// <summary>
		/// Get metadata from the CSDB object.
		/// </summary>
		/// <param name="name">The name of the metadata to get</param>
		/// <returns>
		/// The metadata value
		/// </returns>
		private string GetMetadata(string name)
		{
			return GetMetadataImpl(name);
		}

		/// <summary>
		/// Set the value of a given piece of metadata in the CSDB object.
		/// </summary>
		/// <param name="name">The name of the metadata to set</param>
		/// <param name="val">The new value of the metadata</param>
		private void SetMetadata(string name, string val)
		{
			SetMetadataImpl(name, val);
		}

		/* Metadata exposed as properties: */

		/// <summary>
		/// Code of the CSDB object
		/// </summary>
		public string Code {
			get { return GetMetadata("code"); }
		}

		/// <summary>
		/// Data module code
		/// </summary>
		public string DmCode {
			get { return GetMetadata("dmCode"); }
			set { SetMetadata("dmCode", value); }
		}

		/// <summary>
		/// In-work issue number
		/// </summary>
		public string InWork {
			get { return GetMetadata("inWork"); }
		}

		/// <summary>
		/// Issue number of S1000D the CSDB object conforms to
		/// </summary>
		public string Issue {
			get { return GetMetadata("issue"); }
			set { SetMetadata("issue", value); }
		}

		/// <summary>
		/// The issue date of the CSDB object
		/// </summary>
		public string IssueDate {
			get { return GetMetadata("issueDate"); }
			set { SetMetadata("issueDate", value); }
		}

		/// <summary>
		/// Full issue number of the CSDB object
		/// </summary>
		public string IssueInfo {
			get { return GetMetadata("issueInfo"); }
		}

		/// <summary>
		/// The official issue number of the CSDB object
		/// </summary>
		public string IssueNumber {
			get { return GetMetadata("issueNumber"); }
		}

		/// <summary>
		/// The short S1000D schema name of the CSDB object
		/// </summary>
		public string Schema {
			get { return GetMetadata("schema"); }
		}

		/// <summary>
		/// Check the CSDB object against the appropriate S1000D default BREX.
		/// </summary>
		/// <param name="options">BREX check options</param>
		/// <returns>
		/// An XML report of the results of the BREX check
		/// </returns>
		public XmlDocument CheckAgainstDefaultBREX(BrexCheckOptions options)
		{
			return CheckAgainstDefaultBREXImpl(options);
		}

		/// <summary>
		/// Check the CSDB object against the appropriate S1000D default BREX.
		/// </summary>
		/// <returns>
		/// An XML report of the results of the BREX check
		/// </returns>
		public XmlDocument CheckAgainstDefaultBREX()
		{
			BrexCheckOptions options = new BrexCheckOptions();
			return CheckAgainstDefaultBREXImpl(options);
		}

		/// <summary>
		/// Check the CSDB object against a specified BREX data module.
		/// </summary>
		/// <param name="brex">The BREX data module to check against</para>
		/// <param name="options">BREX check options</param>
		/// <returns>
		/// An XML report of the results of the BREX check
		/// </returns>
		public XmlDocument CheckAgainstBREX(CsdbObject brex, BrexCheckOptions options)
		{
			return CheckAgainstBREXImpl(brex.XmlDocument, options);
		}

		/// <summary>
		/// Check the CSDB object against a specified BREX data module.
		/// </summary>
		/// <param name="brex">The BREX data module to check against</param>
		/// <returns>
		/// An XML report of the results of the BREX check
		/// </returns>
		public XmlDocument CheckAgainstBREX(CsdbObject brex)
		{
			BrexCheckOptions options = new BrexCheckOptions();
			return CheckAgainstBREXImpl(brex.XmlDocument, options);
		}

		/// <summary>
		/// Check the CSDB object against a BREX data module XML file.
		/// </summary>
		/// <param name="path">The path to the BREX file</param>
		/// <param name="options">BREX check options</param>
		/// <returns>
		/// An XML report of the results of the BREX check
		/// </returns>
		public XmlDocument CheckAgainstBREX(string path, BrexCheckOptions options)
		{
			CsdbObject brex = new CsdbObject(path);
			return CheckAgainstBREX(brex, options);
		}

		/// <summary>
		/// Check the CSDB object against a BREX data module XML file.
		/// </summary>
		/// <param name="path">The path to the BREX file</param>
		/// <returns>
		/// An XML report of the results of the BREX check
		/// </returns>
		public XmlDocument CheckAgainstBREX(string path)
		{
			CsdbObject brex = new CsdbObject(path);
			BrexCheckOptions options = new BrexCheckOptions();
			return CheckAgainstBREX(brex, options);
		}

		/// <summary>
		/// Filter the object on a given set of applicability definitions.
		/// </summary>
		/// <param name="app">The applicability definitions to filter on</param>
		/// <param name="mode">The filtering mode to use</param>
		/// <returns>
		/// The filtered CSDB object instance
		/// </returns>
		public CsdbObject Filter(Applicability app, FilterMode mode)
		{
			return FilterImpl(app, mode);
		}
	}
}
