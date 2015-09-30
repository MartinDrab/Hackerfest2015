Unit HiddenRegistryKey;

Interface

Uses
  Windows, DllRegHider, Generics.Collections;

Type
  THiddenRegistryKey = Class
    Private
      FName : WideString;
    Public
      Constructor Create(Var ARecord:REGHIDER_HIDDEN_KEY_RECORD); Reintroduce;

      Class Function Enumerate(AList:TList<THiddenRegistryKey>):Cardinal;

      Property Name : WideString Read FName;
    end;

Implementation

Uses
  Utils;

Function _EnumCallback(Var ARecord:REGHIDER_HIDDEN_KEY_RECORD; AContext:Pointer):LongBool; StdCall;
Var
  key : THiddenRegistryKey;
  list : TList<THiddenRegistryKey>;
begin
list := AContext;
Try
  key := THiddenRegistryKey.Create(ARecord);
  Result := True;
Except
  Result := False;
  End;

If Result Then
  begin
  Try
    list.Add(key);
  Except
    key.Free;
    Result := False;
    end;
  end;
end;

Constructor THiddenRegistryKey.Create(Var ARecord:REGHIDER_HIDDEN_KEY_RECORD);
begin
Inherited Create;
FName := WideCharToString(ARecord.KeyName);
end;

Class Function THiddenRegistryKey.Enumerate(AList:TList<THiddenRegistryKey>):Cardinal;
Var
  key : THiddenRegistryKey;
  tmpList : TList<THiddenRegistryKey>;
begin
tmpList := TList<THiddenRegistryKey>.Create;
Result := HiddenKeysEnum(_EnumCallback, tmpList);
If Result = ERROR_SUCCESS Then
  begin
  For key In tmpList Do
    AList.Add(key);
  end;

tmpList.Free;
end;



End.

