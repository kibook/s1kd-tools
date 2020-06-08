unit S1KDTools;

{$MODE OBJFPC}
{$H+}
{$LINKLIB libs1kd}

interface

uses
  Classes,
  SysUtils,
  DOM;

type
  TFilterMode = (
    fmDefault,
    fmReduce,
    fmSimplify,
    fmPrune
  );

  TBREXCheckOptions = class
  private
    FBits: longint;
    function GetOpt(Opt: longint): Boolean;
    procedure SetOpt(Opt: longint; Enable: Boolean);
    function GetCheckValues: Boolean;
    procedure SetCheckValues(Enable: Boolean);
    function GetCheckSNS: Boolean;
    procedure SetCheckSNS(Enable: Boolean);
    function GetStrictSNS: Boolean;
    procedure SetStrictSNS(Enable: Boolean);
    function GetUnstrictSNS: Boolean;
    procedure SetUnstrictSNS(Enable: Boolean);
    function GetCheckNotations: Boolean;
    procedure SetCheckNotations(Enable: Boolean);
    function GetNormalLog: Boolean;
    procedure SetNormalLog(Enable: Boolean);
    function GetVerboseLog: Boolean;
    procedure SetVerboseLog(Enable: Boolean);
  public
    constructor Create;
    property CheckValues: Boolean read GetCheckValues write SetCheckValues;
    property CheckSNS: Boolean read GetCheckSNS write SetCheckSNS;
    property StrictSNS: Boolean read GetStrictSNS write SetStrictSNS;
    property UnstrictSNS: Boolean read GetUnstrictSNS write SetUnstrictSNS;
    property CheckNotations: Boolean read GetCheckNotations write SetCheckNotations;
    property NormalLog: Boolean read GetNormalLog write SetNormalLog;
    property VerboseLog: Boolean read GetVerboseLog write SetVerboseLog;
  end;

  TApplicability = class
  private
    FData: Pointer;
  public
    constructor Create;
    destructor Destroy; override;
    procedure Assign(Ident, PType, Value: string);
  end;

  TCSDBObject = class
  private
    FXMLDocument: TXMLDocument;
    function GetCode: string;
    function GetCountryISOCode: string;
    procedure SetCountryISOCode(Value: string);
    function GetInWork: string;
    procedure SetInWork(Value: string);
    function GetIssue: string;
    procedure SetIssue(Value: string);
    function GetIssueDate: string;
    procedure SetIssueDate(Value: string);
    function GetIssueNumber: string;
    procedure SetIssueNumber(Value: string);
    function GetLanguageISOCode: string;
    procedure SetLanguageISOCode(Value: string);
  public
    constructor Create(Doc: TXMLDocument);
    constructor Create(Path: string);
    destructor Destroy; override;
    function CheckAgainstDefaultBREX(Options: TBREXCheckOptions): TXMLDocument;
    function CheckAgainstDefaultBREX: TXMLDocument;
    function CheckAgainstBREX(BREX: TCSDBObject; Options: TBREXCheckOptions): TXMLDocument;
    function CheckAgainstBREX(BREX: TCSDBObject): TXMLDocument;
    function CheckAgainstBREX(Path: string; Options: TBREXCheckOptions): TXMLDocument;
    function CheckAgainstBREX(Path: string): TXMLDocument;
    function Filter(App: TApplicability; Mode: TFilterMode): TCSDBObject;
    function GetMetadata(Name: string): string;
    procedure SetMetadata(Name, Value: string);
    property XMLDocument: TXMLDocument read FXMLDocument;
    property Code: string read GetCode;
    property CountryISOCode: string read GetCountryISOCode write SetCountryISOCode;
    property InWork: string read GetInWork write SetInWork;
    property Issue: string read GetIssue write SetIssue;
    property IssueDate: string read GetIssueDate write SetIssueDate;
    property IssueNumber: string read GetIssueNumber write SetIssueNumber;
    property LanguageISOCode: string read GetLanguageISOCode write SetLanguageISOCode;
  end;

implementation

uses
  XMLRead,
  XMLWrite;

const
  S1KD_BREXCHECK_VALUES = 1;
  S1KD_BREXCHECK_SNS = 2;
  S1KD_BREXCHECK_STRICT_SNS = 4;
  S1KD_BREXCHECK_UNSTRICT_SNS = 8;
  S1KD_BREXCHECK_NOTATIONS = 16;
  S1KD_BREXCHECK_NORMAL_LOG = 32;
  S1KD_BREXCHECK_VERBOSE_LOG = 64;

function s1kdCheckDefaultBREX(const object_xml: pchar; object_size: longint; options: longint; report_xml: ppchar; report_size: plongint): longint; cdecl; external;
function s1kdCheckBREX(const object_xml: pchar; object_size: longint; const brex_xml: pchar; brex_size: longint; options: longint; report_xml: ppchar; report_size: plongint): longint; cdecl; external;
function s1kdGetMetadata(const object_xml: pchar; object_size: longint; const name: pchar): pchar; cdecl; external;
function s1kdSetMetadata(const object_xml: pchar; object_size: longint; const name: pchar; const value: pchar; result_xml: ppchar; result_size: plongint): longint; cdecl; external;
function s1kdNewApplicability: pointer; cdecl; external;
procedure s1kdFreeApplicability(app: pointer); cdecl; external;
procedure s1kdAssign(app: pointer; ident: pchar; type_: pchar; value: pchar); cdecl; external;
function s1kdFilter(const object_xml: pchar; object_size: longint; app: pointer; mode: TFilterMode; result_xml: ppchar; result_size: plongint): longint; cdecl; external;

constructor TBREXCheckOptions.Create;
begin
  FBits := 0
end;

function TBREXCheckOptions.GetOpt(Opt: longint): Boolean;
begin
  Result := (FBits and Opt) = Opt
end;

procedure TBREXCheckOptions.SetOpt(Opt: longint; Enable: Boolean);
begin
  if Enable then
    FBits := FBits or Opt
  else
    FBits := FBits and not Opt
end;

function TBREXCheckOptions.GetCheckValues: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_VALUES)
end;

procedure TBREXCheckOptions.SetCheckValues(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_VALUES, Enable)
end;

function TBREXCheckOptions.GetCheckSNS: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_SNS)
end;

procedure TBREXCheckOptions.SetCheckSNS(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_SNS, Enable)
end;

function TBREXCheckOptions.GetStrictSNS: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_STRICT_SNS)
end;

procedure TBREXCheckOptions.SetStrictSNS(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_STRICT_SNS, Enable)
end;

function TBREXCheckOptions.GetUnstrictSNS: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_UNSTRICT_SNS)
end;

procedure TBREXCheckOptions.SetUnstrictSNS(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_UNSTRICT_SNS, Enable)
end;

function TBREXCheckOptions.GetCheckNotations: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_NOTATIONS)
end;

procedure TBREXCheckOptions.SetCheckNotations(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_NOTATIONS, Enable)
end;

function TBREXCheckOptions.GetNormalLog: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_NORMAL_LOG)
end;

procedure TBREXCheckOptions.SetNormalLog(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_NORMAL_LOG, Enable)
end;

function TBREXCheckOptions.GetVerboseLog: Boolean;
begin
  Result := GetOpt(S1KD_BREXCHECK_VERBOSE_LOG)
end;

procedure TBREXCheckOptions.SetVerboseLog(Enable: Boolean);
begin
  SetOpt(S1KD_BREXCHECK_VERBOSE_LOG, Enable)
end;

constructor TApplicability.Create;
begin
  FData := s1kdNewApplicability
end;

destructor TApplicability.Destroy;
begin
  s1kdFreeApplicability(FData)
end;

procedure TApplicability.Assign(Ident, PType, Value: string);
begin
  s1kdAssign(FData, PChar(Ident), PChar(PType), PChar(Value))
end;

constructor TCSDBObject.Create(Doc: TXMLDocument);
begin
  FXMLDocument := Doc
end;

constructor TCSDBObject.Create(Path: string);
begin
  ReadXMLFile(FXMLDocument, Path)
end;

destructor TCSDBObject.Destroy;
begin
  FXMLDocument.Free;
end;

function TCSDBObject.CheckAgainstDefaultBREX(Options: TBREXCheckOptions): TXMLDocument;
var
  IStream: TStringStream;
  OStream: TStringStream;
  IStr: string;
  OStr: PChar;
  Size: longint;
begin
  IStream := TStringStream.Create('');
  WriteXMLFile(FXMLDocument, IStream);
  IStr := IStream.DataString;
  IStream.Free;

  s1kdCheckDefaultBREX(PChar(IStr), Length(IStr), Options.FBits, @OStr, @Size);

  OStream := TStringStream.Create(OStr);
  StrDispose(OStr);
  ReadXMLFile(Result, OStream);
  OStream.Free
end;

function TCSDBObject.CheckAgainstDefaultBREX: TXMLDocument;
var
  Options: TBREXCheckOptions;
begin
  Options := TBREXCheckOptions.Create;
  Result := CheckAgainstDefaultBREX(Options);
  Options.Free
end;

function TCSDBObject.CheckAgainstBREX(BREX: TCSDBObject; Options: TBREXCheckOptions): TXMLDocument;
var
  IStream: TStringStream;
  BStream: TStringStream;
  OStream: TStringStream;
  IStr: string;
  BStr: string;
  OStr: PChar;
  Size: longint;
begin
  IStream := TStringStream.Create('');
  WriteXMLFile(FXMLDocument, IStream);
  IStr := IStream.DataString;
  IStream.Free;

  BStream := TStringStream.Create('');
  WriteXMLFile(BREX.XMLDocument, BStream);
  BStr := BStream.DataString;
  BStream.Free;

  s1kdCheckBREX(PChar(IStr), Length(IStr), PChar(BStr), Length(BStr), Options.FBits, @OStr, @Size);

  OStream := TStringStream.Create(OStr);
  StrDispose(OStr);
  ReadXMLFile(Result, OStream);
  OStream.Free
end;

function TCSDBObject.CheckAgainstBREX(BREX: TCSDBObject): TXMLDocument;
var
  Options: TBREXCheckOptions;
begin
  Options := TBREXCheckOptions.Create;
  Result := CheckAgainstBREX(BREX, Options);
  Options.Free
end;

function TCSDBObject.CheckAgainstBREX(Path: string; Options: TBREXCheckOptions): TXMLDocument;
var
  BREX: TCSDBObject;
begin
  BREX := TCSDBObject.Create(Path);
  Result := CheckAgainstBREX(BREX, Options)
end;

function TCSDBObject.CheckAgainstBREX(Path: string): TXMLDocument;
var
  Options: TBREXCheckOptions;
begin
  Options := TBREXCheckOptions.Create;
  Result := CheckAgainstBREX(Path, Options)
end;

function TCSDBObject.Filter(App: TApplicability; Mode: TFilterMode): TCSDBObject;
var
  IStream: TStringStream;
  OStream: TStringStream;
  IStr: string;
  OStr: PChar;
  Size: longint;
  Doc: TXMLDocument;
  Err: longint;
begin
  IStream := TStringStream.Create('');
  WriteXMLFile(FXMLDocument, IStream);
  IStr := IStream.DataString;
  IStream.Free;

  Err := s1kdFilter(PChar(IStr), Length(IStr), App.FData, Mode, @OStr, @Size);

  if Err <> 0 then
    raise Exception.Create('Filtering failed');

  OStream := TStringStream.Create(OStr);
  strDispose(OStr);
  ReadXMLFile(Doc, OStream);
  Result := TCSDBObject.Create(Doc);
  OStream.Free
end;

function TCSDBObject.GetMetadata(Name: string): string;
var
  IStream: TStringStream;
  IStr: string;
begin
  IStream := TStringStream.Create('');
  WriteXMLFile(FXMLDocument, IStream);
  IStr := IStream.DataString;
  IStream.Free;

  Result := s1kdGetMetadata(PChar(IStr), Length(IStr), PChar(Name));
end;

procedure TCSDBOBject.SetMetadata(Name, Value: string);
var
  IStream: TStringStream;
  OStream: TStringStream;
  IStr: string;
  OStr: PChar;
  Size: longint;
  Err: longint;
begin
  IStream := TStringStream.Create('');
  WriteXMLFile(FXMLDocument, IStream);
  IStr := IStream.DataString;
  IStream.Free;

  Err := s1kdSetMetadata(PChar(IStr), Length(IStr), PChar(Name), PChar(Value), @OStr, @Size);

  if Err <> 0 then
    raise Exception.Create('Set metadata failed');

  OStream := TStringStream.Create(OStr);
  StrDispose(OStr);
  FXMLDocument.Free;
  ReadXMLFile(FXMLDocument, OStream);
  OStream.Free
end;

function TCSDBObject.GetCode: string;
begin
  Result := GetMetadata('code')
end;

function TCSDBObject.GetCountryISOCode: string;
begin
  Result := GetMetadata('countryIsoCode')
end;

procedure TCSDBObject.SetCountryISOCode(Value: string);
begin
  SetMetadata('countryIsoCode', Value)
end;

function TCSDBObject.GetInWork: string;
begin
  Result := GetMetadata('inWork')
end;

procedure TCSDBObject.SetInWork(Value: string);
begin
  SetMetadata('inWork', Value)
end;

function TCSDBObject.GetIssue: string;
begin
  Result := GetMetadata('issue')
end;

procedure TCSDBObject.SetIssue(Value: string);
begin
  SetMetadata('issue', Value)
end;

function TCSDBObject.GetIssueDate: string;
begin
  Result := GetMetadata('issueDate')
end;

procedure TCSDBObject.SetIssueDate(Value: string);
begin
  SetMetadata('issueDate', Value)
end;

function TCSDBObject.GetIssueNumber: string;
begin
  Result := GetMetadata('issueNumber')
end;

procedure TCSDBObject.SetIssueNumber(Value: string);
begin
  SetMetadata('issueNumber', Value)
end;

function TCSDBObject.GetLanguageISOCode: string;
begin
  Result := GetMetadata('languageIsoCode')
end;

procedure TCSDBObject.SetLanguageISOCode(Value: string);
begin
  SetMetadata('languageIsoCode', Value)
end;

end.
