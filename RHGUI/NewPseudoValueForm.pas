Unit NewPseudoValueForm;

Interface

Uses
  Winapi.Windows, Winapi.Messages, System.SysUtils,
  System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.ExtCtrls,
  Vcl.StdCtrls, Vcl.ComCtrls, DllRegHider, PseudoRegistryValue;

Type
  TNewPseudoValueFrm = Class (TForm)
    MainPanel: TPanel;
    OkButton: TButton;
    StornoButton: TButton;
    ExtraInformationGroupBox: TGroupBox;
    DataGroupBox: TGroupBox;
    BasicInformationGroupBox: TGroupBox;
    KeyNameEdit: TEdit;
    ValueNameEdit: TEdit;
    ProcessNameEdit: TEdit;
    ValueTypeComboBox: TComboBox;
    ChangeModeComboBox: TComboBox;
    DeleteModeComboBox: TComboBox;
    Label1: TLabel;
    Label2: TLabel;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    ValueDataRichEdit: TRichEdit;
    procedure FormCreate(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure StornoButtonClick(Sender: TObject);
    procedure OkButtonClick(Sender: TObject);
  Private
    FCancelled : Boolean;
    FKeyName : WideString;
    FValueName : WideString;
    FValueType : Cardinal;
    FValueData : Pointer;
    FValueDataLength : Cardinal;
    FProcessName : WideString;
    FChangeMode : ERegistryValueOpMode;
    FDeleteMode : ERegistryValueOpMode;
    FPseudoValue : TPseudoRegistryValue;
  Public
    Constructor Create(AOwner:TComponent; AValue:TPseudoRegistryValue = Nil); Reintroduce;

    Property Cancelled : Boolean Read FCancelled;
    Property KeyName : WideString Read FKeyName;
    Property ValueName : WideString Read FValueName;
    Property ValueType : Cardinal Read FValueType;
    Property ValueData : Pointer Read FValueData;
    Property ValueDataLength : Cardinal Read FValueDataLength;
    Property ProcessName : WideString Read FProcessName;
    Property ChangeMode : ERegistryValueOpMode Read FChangeMode;
    Property DeleteMode : ERegistryValueOpMode Read FDeleteMode;
  end;


Implementation

{$R *.DFM}

Uses
  Utils;

Constructor TNewPseudoValueFrm.Create(AOwner:TComponent; AValue:TPseudoRegistryValue = Nil);
begin
FPseudoValue := AValue;
Inherited Create(AOwner);
end;

Procedure TNewPseudoValueFrm.FormCreate(Sender: TObject);
begin
FValueData := Nil;
FValueDataLength := 0;
FCancelled := True;
If Assigned(FPseudoValue) Then
  begin
  KeyNameEdit.Text := FPseudoValue.KeyName;
  ValueNameEdit.Text := FPseudoValue.ValueName;
  ValuetypeComboBox.ItemIndex := 0;
  Case FPseudoValue.ValueType Of
    REG_NONE : ValuetypeComboBox.ItemIndex := 0;
    REG_BINARY : ValuetypeComboBox.ItemIndex := 1;
    REG_DWORD : ValuetypeComboBox.ItemIndex := 2;
    11 : ValuetypeComboBox.ItemIndex := 3;
    REG_SZ : ValuetypeComboBox.ItemIndex := 4;
    REG_EXPAND_SZ : ValuetypeComboBox.ItemIndex := 5;
    REG_MULTI_SZ : ValuetypeComboBox.ItemIndex := 6;
    end;

  Case FPseudoValue.ValueType Of
    REG_SZ,
    REG_EXPAND_SZ : ValueDataRichEdit.Text := WideCharToString(FPseudoValue.Data);
    REG_MULTI_SZ : MultiStringToStringList(FPseudoValue.Data, ValueDataRichEdit.Lines);
    REG_DWORD : begin
      If FPseudoValue.DataLength = SizeOf(Cardinal) THen
        ValueDataRichEdit.Text := IntToStr(PInteger(FPseudoValue.Data)^);
      end;
    11 : begin
      If FPseudoValue.DataLength = SizeOf(Int64) THen
        ValueDataRichEdit.Text := IntToStr(PInt64(FPseudoValue.Data)^);
      end;
    Else ValueDataRichEdit.Text := BinaryDataToString(FPseudoValue.Data, FPseudoValue.DataLength);
    end;

  ChangeModeComboBox.ItemIndex := Ord(FPseudoValue.ChangeMode);
  DeleteModeComboBox.ItemIndex := Ord(FPseudoValue.DeleteMode);
  ProcessNameEdit.Text := FPseudoValue.ProcessName;
  end;
end;

Procedure TNewPseudoValueFrm.FormDestroy(Sender: TObject);
begin
If Assigned(FValueData) Then
  FreeMem(FValueData);
end;

Procedure TNewPseudoValueFrm.OkButtonClick(Sender: TObject);
Var
  dw : Cardinal;
  qw : UInt64;
begin
FCancelled := False;
FKeyName := KeyNameEdit.Text;
FValueName := ValueNameEdit.Text;
Case ValueTypeComboBox.ItemIndex Of
  0 : FValueType := REG_NONE;
  1 : FValueType := REG_BINARY;
  2 : FValueType := REG_DWORD;
  3 : FValueType := 11;
  4 : FValueType := REG_SZ;
  5 : FValueType := REG_EXPAND_SZ;
  6 : FValueType := REG_MULTI_SZ;
  end;

Case FValueType Of
  REG_SZ,
  REG_EXPAND_SZ : begin
    FValueDataLength := (Length(ValueDataRichEdit.Text) + 1)*Sizeof(WideChar);
    FValueData := AllocMem(FValueDataLength);
    FCancelled := Not Assigned(FValueData);
    If Not FCancelled Then
      CopyMemory(FValueData, PWideChar(ValueDataRichEdit.Text), FValueDataLength);
    end;
  REG_DWORD : begin
    FValueDataLength := SizeOf(Cardinal);
    FValueData := AllocMem(FValueDataLength);
    FCancelled := Not Assigned(FValueData);
    If Not FCancelled Then
      begin
      try
        dw := Cardinal(StrToInt64(ValueDataRichEdit.Text));
        CopyMemory(FValueData, @dw, FValueDataLength);
      Except
        FCancelled := True;
        end;

      If FCancelled Then
        FreeMem(FValueData);
      end;
    end;
  11 : begin
    FValueDataLength := SizeOf(UInt64);
    FValueData := AllocMem(FValueDataLength);
    FCancelled := Not Assigned(FValueData);
    If Not FCancelled Then
      begin
      try
        qw := UInt64(StrToInt64(ValueDataRichEdit.Text));
        CopyMemory(FValueData, @qw, FValueDataLength);
      Except
        FCancelled := True;
        end;

      If FCancelled Then
        FreeMem(FValueData);
      end;
    end;
  REG_MULTI_SZ : begin
    FCancelled := Not StringListToMultiStringData(ValueDataRichEdit.Lines, FValueData, FValueDatalength)
    end;
  REG_BINARY,
  REG_NONE : begin
     FCancelled := Not StringToBinaryData(ValueDataRichEdit.Text, FValueData, FValueDataLength);
    end;
  end;

FProcessName := ProcessnameEdit.Text;
FChangeMode := ERegistryValueOpMode(ChangeModeComboBox.ItemIndex);
FDeleteMode := ERegistryValueOpMode(DeleteModeComboBox.ItemIndex);
If Not FCancelled Then
  Close
Else begin
  FValuedata := Nil;
  FValueDataLength := 0;
  end;
end;

Procedure TNewPseudoValueFrm.StornoButtonClick(Sender: TObject);
begin
Close;
end;

End.
