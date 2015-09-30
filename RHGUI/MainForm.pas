Unit MainForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants,
  System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls, Vcl.ExtCtrls,
  Vcl.ComCtrls,
  Generics.Collections, HiddenRegistryKey, PseudoRegistryValue;

Type
  TForm1 = Class (TForm)
    HiddenRegistryKeysGroupBox: TGroupBox;
    PseudoRegistryValuesGroupBox: TGroupBox;
    HiddenRegistryKeysPanel: TPanel;
    PseudoRegistryValuesPanel: TPanel;
    HiddenKeysAddButton: TButton;
    HiddenKeysDeleteButton: TButton;
    HiddenKeysRefreshButton: TButton;
    PseudoValuesAddButton: TButton;
    PseudoValuesDeleteButton: TButton;
    PseudoValuesRefreshButton: TButton;
    HiddenKeysListView: TListView;
    PseudoValuesListView: TListView;
    PseudoValueEditButton: TButton;
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure AddButtonClick(Sender: TObject);
    procedure RefreshButtonClick(Sender: TObject);
    procedure DeleteButtonClick(Sender: TObject);
    procedure ListViewData(Sender: TObject; Item: TListItem);
  Private
    FHiddenkeyList : TList<THiddenRegistryKey>;
    FPSeudoValueList : TList<TPseudoRegistryValue>;

    Procedure HiddenKeysRefresh;
    Procedure PseudoValuesRefresh;
  end;

Var
  Form1: TForm1;

Implementation

Uses
  Utils, DllRegHider, NewPseudoValueForm;

{$R *.DFM}

Procedure TForm1.HiddenKeysRefresh;
Var
  key : THiddenRegistryKey;
  err : Cardinal;
  tmpList : TList<THiddenRegistryKey>;
  tmpList2 : TList<THiddenRegistryKey>;
begin
tmpList := TList<THiddenRegistryKey>.Create;
err := THiddenRegistryKey.Enumerate(tmpList);
If err = ERROR_SUCCESS Then
  begin
  HiddenKeysListView.Items.Count := 0;
  tmpList2 := FHiddenKeyList;
  FHiddenKeyList := tmpList;
  tmpList := tmpList2;
  HiddenKeysListView.Items.Count := FHiddenKeyList.Count;
  end
Else WindowsErrorMessage(err, 'Failed to enumerate hidden subkeys', []);

For key In tmplist Do
  key.Free;

tmpList.Free;
end;

Procedure TForm1.ListViewData(Sender: TObject; Item: TListItem);
Var
  value : TPseudoRegistryValue;
  key : THiddenRegistryKey;
begin
If Sender = HiddenKeysListView Then
  begin
  With Item Do
    begin
    key := FHiddenkeyList[Index];
    Caption := key.Name;
    end;
  end
Else If Sender = PseudoValuesListView Then
  begin
  With Item Do
    begin
    value := FPseudoValueList[Index];
    Caption := value.KeyName;
    SubItems.Add(value.ValueName);
    SubItems.Add(RegistryValueTypeToStr(value.ValueType));
    SubItems.Add(Format('%s | %s', [RegistryValueOpModeToStr(value.ChangeMode), RegistryValueOpModeToStr(value.DeleteMode)]));
    SubItems.Add('Not implemented');
    SubItems.Add(value.ProcessName);
    end;
  end;
end;

Procedure TForm1.PseudoValuesRefresh;
Var
  value : TPseudoRegistryValue;
  err : Cardinal;
  tmpList : TList<TPseudoRegistryValue>;
  tmpList2 : TList<TPseudoRegistryValue>;
begin
tmpList := TList<TPseudoRegistryValue>.Create;
err := TPseudoRegistryValue.Enumerate(tmpList);
If err = ERROR_SUCCESS Then
  begin
  PseudoValuesListView.Items.Count := 0;
  tmpList2 := FPseudoValueList;
  FPseudoValueList := tmpList;
  tmpList := tmpList2;
  PseudoValuesListView.Items.Count := FPseudoValueList.Count;
  end
Else WindowsErrorMessage(err, 'Failed to enumerate pseudo values', []);

For value In tmplist Do
  value.Free;

tmpList.Free;
end;

Procedure TForm1.RefreshButtonClick(Sender: TObject);
begin
If Sender = HiddenKeysRefreshButton Then
  HiddenKeysRefresh
Else If Sender = PseudoValuesRefreshButton Then
  PseudoValuesRefresh;
end;

Procedure TForm1.AddButtonClick(Sender: TObject);
Var
  err : Cardinal;
  kn : WideString;
  L : TListItem;
begin
If Sender = HiddenKeysAddButton Then
  begin
  kn := InputBox('Hide a key', 'Full key name', '');
  If kn <> '' Then
    begin
    err := HiddenKeyAdd(PWideChar(kn));
    If err = ERROR_SUCCESS Then
      HiddenKeysRefresh;

    If err <> ERROR_SUCCESS Then
      WindowsErrorMessage(err, '', []);
    end;
  end
Else If Sender = PseudoValuesAddButton Then
  begin
  With TNewPseudoValueFrm.Create(Application) Do
    begin
    ShowModal;
    If Not Cancelled Then
      begin
      err := PseudoValueAdd(PWideChar(KeyName), PWideChar(ValueName), ValueType, ValueData, ValueDataLength, DeleteMode, ChangeMode, PWideChar(ProcessName));
      If err = ERROR_SUCCESS Then
        PseudoValuesRefresh;

      If err <> ERROR_SUCCESS Then
        WindowsErrorMessage(err, '', []);
      end;

    Free;
    end;
  end
Else If Sender = PseudoValueEditButton Then
  begin
  L := PseudoValuesListView.Selected;
  If Assigned(L) Then
    begin
    With TNewPseudoValueFrm.Create(Application, FPseudoValueList[L.Index]) Do
      begin
      ShowModal;
      If Not Cancelled Then
        begin
        err := PseudoValueSet(PWideChar(KeyName), PWideChar(ValueName), ValueType, ValueData, ValueDataLength, DeleteMode, ChangeMode, PWideChar(ProcessName));
        If err = ERROR_SUCCESS Then
          PseudoValuesRefresh;

        If err <> ERROR_SUCCESS Then
          WindowsErrorMessage(err, '', []);
        end;

      Free;
      end;
    end;
  end;
end;

Procedure TForm1.DeleteButtonClick(Sender: TObject);
Var
  err : Cardinal;
  value : TPseudoRegistryValue;
  key : THiddenRegistryKey;
  L : TListItem;
begin
err := ERROR_SUCCESS;
If Sender = HiddenKeysDeleteButton Then
  begin
  L := HiddenKeysListView.Selected;
  If Assigned(L) Then
    begin
    key := FHiddenKeyList[L.Index];
    err := HiddenKeyDelete(PWideChar(key.Name));
    If err = ERROR_SUCCESS Then
      HiddenKeysRefresh;
    end;
  end
Else If Sender = PseudoValuesDeleteButton Then
  begin
  L := PseudoValuesListView.Selected;
  If Assigned(L) Then
    begin
    value := FPseudoValueList[L.Index];
    err := PseudoValueDelete(PWideChar(value.KeyName), PWideChar(value.ValueName));
    If err = ERROR_SUCCESS Then
      PseudoValuesRefresh;
    end;
  end;

If err <> ERROR_SUCCESS Then
  WindowsErrorMessage(err, '', []);
end;

Procedure TForm1.FormClose(Sender: TObject; var Action: TCloseAction);
Var
  key : THiddenRegistryKey;
  value : TPseudoRegistryValue;
begin
PseudoValuesListView.Items.Count := 0;
For value In FPseudoValueList Do
  value.Free;

FPseudoValueList.Free;
HiddenKeysListView.Items.Count := 0;
For key In FHiddenkeyList Do
  key.Free;

FHiddenKeyList.Free;
end;

Procedure TForm1.FormCreate(Sender: TObject);
begin
FHiddenKeyList := TList<THiddenRegistryKey>.Create;
FPseudoValueList := TList<TPseudoRegistryValue>.Create;
HiddenKeysRefresh;
PseudoValuesRefresh;
end;

End.

