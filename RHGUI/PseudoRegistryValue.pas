Unit PseudoRegistryValue;

Interface

Uses
  Windows, DllRegHider, Generics.Collections;

Type
  TPseudoRegistryValue = Class
    Private
      FKeyName : WideString;
      FValueName : WideString;
      FValueType : Cardinal;
      FData : Pointer;
      FDataLength : Cardinal;
      FChangeMode : ERegistryValueOpMode;
      FDeleteMode : ERegistryValueOpMode;
      FProcessName : WideString;
    Public
      Constructor Create(Var ARecord:REGHIDER_PSEUDO_VALUE_RECORD); Reintroduce;
      Destructor Destroy; Override;

      Class Function Enumerate(AList:TList<TPseudoRegistryValue>):Cardinal;

      Property KeyName : WideString Read FKeyName;
      Property ValueName : WideString Read FValueName;
      Property ValueType : Cardinal Read FValueType;
      Property Data : Pointer Read FData;
      Property DataLength : Cardinal Read FDataLength;
      Property ChangeMode : ERegistryValueOpMode Read FChangeMode;
      Property DeleteMode : ERegistryValueOpMode Read FDeleteMode;
      Property ProcessName : WideString Read FProcessName;
    end;


Implementation

Uses
  SysUtils;

Function _EnumCallback(Var ARecord:REGHIDER_PSEUDO_VALUE_RECORD; AContext:Pointer):LongBool; StdCall;
Var
  value : TPseudoRegistryValue;
  list : TList<TPseudoRegistryValue>;
begin
list := AContext;
Try
  value := TPseudoRegistryValue.Create(ARecord);
  Result := True;
  Except
  Result := False;
  End;

If Result Then
  begin
  Try
    list.Add(value);
  Except
    value.Free;
    Result := False;
    end;
  end;
end;

Constructor TPseudoRegistryValue.Create(Var ARecord:REGHIDER_PSEUDO_VALUE_RECORD);
begin
Inherited Create;
FData := Nil;
FKeyName := WideCharToString(ARecord.KeyName);
FValueName := WideCharToString(ARecord.ValueName);
FProcessName := WideCharToString(ARecord.ProcessName);
FValueType := ARecord.ValueType;
FChangeMode := ARecord.ChangeMode;
FDeleteMode := ARecord.DeleteMode;
FDataLength := ARecord.DataLength;
If FDataLength > 0 Then
  begin
  FData := HeapAlloc(GetProcessHeap, HEAP_ZERO_MEMORY, FDataLength);
  If Not Assigned(FData) Then
    Raise Exception.Create('Out of memory');

  CopyMemory(FData, ARecord.Data, FDataLength);
  end;
end;

Destructor TPseudoRegistryValue.Destroy;
begin
If Assigned(FData) Then
  HeapFree(GetProcessHeap, 0, FData);

Inherited Destroy;
end;



Class Function TPseudoRegistryValue.Enumerate(AList:TList<TPseudoRegistryValue>):Cardinal;
Var
  value : TPseudoRegistryValue;
  tmpList : TList<TPseudoRegistryValue>;
begin
tmpList := TList<TPseudoRegistryValue>.Create;
Result := PseudoValuesEnum(_EnumCallback, tmpList);
If Result = ERROR_SUCCESS Then
  begin
  For value In tmpList Do
    AList.Add(value);
  end;

tmpList.Free;
end;



End.
