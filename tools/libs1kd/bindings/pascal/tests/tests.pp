program test;

uses
  Classes,
  SysUtils,
  DOM,
  XMLWrite,
  S1KDTools;

procedure PrintResults(Report: TXMLDocument);
var
  S: TStringStream;
begin
  S := TStringStream.Create('');
  WriteXMLFile(Report, S);
  writeln(S.DataString)
end;

var
  DM: TCSDBObject;
  Report: TXMLDocument;
  Options: TBREXCheckOptions;
  App: TApplicability;
  Instance: TCSDBObject;
begin
  DM := TCSDBObject.Create('test.xml');

  { BREX check }
  Options := TBREXCheckOptions.Create;
  Options.CheckValues := true;

  Report := DM.CheckAgainstDefaultBREX;
  PrintResults(Report);
  Report.Free;

  Report := DM.CheckAgainstDefaultBREX(Options);
  PrintResults(Report);
  Report.Free;

  Report := DM.CheckAgainstBREX('brex.xml');
  PrintResults(Report);
  Report.Free;

  Options.Free;

  { Filtering }
  App := TApplicability.Create;
  App.Assign('version', 'prodattr', 'A');

  Instance := DM.Filter(App, fmDefault);
  PrintResults(Instance.XMLDocument);
  Instance.Free;

  Instance := DM.Filter(App, fmReduce);
  PrintResults(Instance.XMLDocument);
  Instance.Free;

  App.Free;

  { Metadata }
  writeln('Code: ', DM.Code);

  writeln('Issue Number: ', DM.IssueNumber);
  writeln('InWork: ', DM.InWork);

  writeln('Language: ', DM.LanguageISOCode);
  writeln('Country: ', DM.CountryISOCode);

  writeln('Issue: ', DM.Issue);
  DM.Issue := '4.1';
  writeln('Issue: ', DM.Issue);

  writeln('Issue date: ', DM.IssueDate);
  DM.IssueDate := '1970-01-01';
  writeln('Issue date: ', DM.IssueDate);

  DM.Free
end.
