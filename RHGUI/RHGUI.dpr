program RHGUI;

uses
  Windows,
  Vcl.Forms,
  MainForm in 'MainForm.pas' {Form1},
  DllRegHider in '..\include\DllRegHider.pas',
  Utils in 'Utils.pas',
  HiddenRegistryKey in 'HiddenRegistryKey.pas',
  PseudoRegistryValue in 'PseudoRegistryValue.pas',
  NewPseudoValueForm in 'NewPseudoValueForm.pas' {NewPseudoValueFrm};

{$R *.res}

Var
  err : Cardinal;
begin
Application.Initialize;
err := DLLregHider.Init;
If err = ERROR_SUCCESS Then
  begin
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
  DllRegHider.Finit;
  end
Else WindowsErrorMessage(err, 'Failed to initialize reghider.dll', []);
end.

