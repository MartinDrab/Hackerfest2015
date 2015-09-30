Unit Utils;

Interface

Uses
  Windows, DllRegHider, Classes;

Function RegistryValueOpModeToStr(AMode:ERegistryValueOpMode):WideString;
Function RegistryValueTypeToStr(AType:Cardinal):WideString;
Function StringListToMultiStringData(AList:TStrings; Var AData:Pointer; Var ADataLength:Cardinal):Boolean;
Procedure MultiStringToStringList(AM:PWideChar; AList:TStrings);
Function StringToBinaryData(S:WideString; Var AData:Pointer; Var ADataLength:Cardinal):Boolean;
Function BinaryDataToString(ABuffer:Pointer; ASize:Cardinal):WideString;

Function WideCharToString(AWideChar:PWideChar):WideString;
Procedure WindowsErrorMessage(AErrorCode:Cardinal; AMessage:WideString; AArgs:Array Of Const);

Implementation

Uses
  SysUtils;

Function RegistryValueOpModeToStr(AMode:ERegistryValueOpMode):WideString;
begin
Case AMode Of
  rvdmDeny: Result := 'Deny';
  rvdmAllow: Result := 'Allow';
  rvdmPretend: Result := 'Pretend';
  Else Result := Format('<unknown> (%d)', [Ord(AMode)]);
  end;
end;

Function RegistryValueTypeToStr(AType:Cardinal):WideString;
begin
Case AType Of
  REG_NONE : Result := 'REG_NONE';
  REG_DWORD : Result := 'REG_DWORD';
  REG_SZ : Result := 'REG_SZ';
  REG_EXPAND_SZ : Result := 'REG_EXPAND_SZ';
  REG_MULTI_SZ : Result := 'REG_MULTI_SZ';
  REG_BINARY : Result := 'REG_BINARY';
  11 : Result := 'REG_QWORD';
  Else Result := Format('<not supported> (%d)', [AType]);
  end;
end;

Function WideCharToString(AWideChar:PWideChar):WideString;
begin
Result := Copy(WideString(AWideChar), 1, StrLen(AWideChar));
end;

Procedure WindowsErrorMessage(AErrorCode:Cardinal; AMessage:WideString; AArgs:Array Of Const);
Var
  wholeMsg : WideString;
  errString : WideString;
begin
errString := Format(': %s (%d)', [SysErrorMessage(AErrorCode), AErrorCode]);
wholeMsg := Format(AMessage, AArgs) + errString;
MessageBoxW(0, PWideChar(wholeMsg), 'Error', MB_OK Or MB_ICONERROR);
end;

Function StringListToMultiStringData(AList:TStrings; Var AData:Pointer; Var ADataLength:Cardinal):Boolean;
Var
  len : Cardinal;
  tmp : PWideChar;
  I : Integer;
begin
ADataLength := SizeOf(WideChar);
For I := 0 To AList.Count - 1 Do
  Inc(ADataLength, (Length(AList[I]) + 1)*SizeOf(WideChar));

AData := AllocMem(ADataLength);
Result := Assigned(AData);
If Result Then
  begin
  tmp := AData;
  For I := 0 To AList.Count - 1 Do
    begin
    len := Length(AList[I]);
    CopyMemory(tmp, PWideChar(AList[I]), (len + 1)*SizeOf(WiDeChar));
    Inc(tmp, len + 1);
    end;

  tmp^ := #0;
  end;
end;

Procedure MultiStringToStringList(AM:PWideChar; AList:TStrings);
Var
  len : Cardinal;
  tmp : PWideChar;
begin
tmp := AM;
While tmp^ <> #0 Do
  begin
  len := Strlen(tmp);
  AList.Add(WideCharToString(tmp));
  Inc(tmp, len + 1);
  end;
end;

Function StringToBinaryData(S:WideString; Var AData:Pointer; Var ADataLength:Cardinal):Boolean;
Var
  d : WideChar;
  b : Byte;
  value : Byte;
  p : PByte;
  I : Integer;
begin
Result := (Length(S) Mod 2) = 0;
If Result Then
  begin
  For I := 1 To Length(S) Do
    begin
    Result := (
      ((S[I] >= '0') And (S[I] <= '9')) Or
      ((S[I] >= 'a') And (S[I] <= 'f')) Or
      ((S[I] >= 'A') And (S[I] <= 'F'))
      );
    If Not Result Then
      Break;
    end;

  If Result Then
    begin
    ADataLength := Length(S) Div 2;
    AData := AllocMem(ADataLength);
    If Assigned(AData) Then
      begin
      p := AData;
      For I := 0 To ADataLength - 1 Do
        begin
        d := S[2*I + 1];
        If ((d >= '0') And (d <= '9')) Then
          b := Ord(d) - Ord('0')
        Else If ((d >= 'a') And (d <= 'f')) Then
          b := Ord(d) - Ord('a') + 10
        Else b := Ord(d) - Ord('A') + 10;

        value := b;

        d := S[2*I+2];
        If ((d >= '0') And (d <= '9')) Then
          b := Ord(d) - Ord('0')
        Else If ((d >= 'a') And (d <= 'f')) Then
          b := Ord(d) - Ord('a') + 10
        Else b := Ord(d) - Ord('A') + 10;

        value := value + 16*b;
        p^ := value;
        Inc(p);
        end;
      end;
    end;
  end;
end;

Function BinaryDataToString(ABuffer:Pointer; ASize:Cardinal):WideString;
Var
  p : PByte;
  I : Integer;
begin
Result := '';
p := ABuffer;
For I := 0 To ASize - 1 Do
  begin
  Result := Result + IntToHex(p^, 2);
  Inc(p);
  end;
end;

End.

