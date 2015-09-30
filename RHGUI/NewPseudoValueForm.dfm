object NewPseudoValueFrm: TNewPseudoValueFrm
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  Caption = 'NewPseudoValueFrm'
  ClientHeight = 341
  ClientWidth = 333
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object MainPanel: TPanel
    Left = 0
    Top = 0
    Width = 333
    Height = 306
    Align = alTop
    TabOrder = 0
    object ExtraInformationGroupBox: TGroupBox
      Left = 1
      Top = 188
      Width = 331
      Height = 111
      Align = alTop
      Caption = 'Extra information'
      TabOrder = 0
      object Label4: TLabel
        Left = 3
        Top = 20
        Width = 66
        Height = 13
        Caption = 'Process name'
      end
      object Label5: TLabel
        Left = 3
        Top = 43
        Width = 66
        Height = 13
        Caption = 'Change mode'
      end
      object Label6: TLabel
        Left = 3
        Top = 70
        Width = 60
        Height = 13
        Caption = 'Delete mode'
      end
      object ProcessNameEdit: TEdit
        Left = 72
        Top = 16
        Width = 184
        Height = 21
        TabOrder = 0
      end
      object ChangeModeComboBox: TComboBox
        Left = 72
        Top = 43
        Width = 105
        Height = 21
        Style = csDropDownList
        ItemIndex = 0
        TabOrder = 1
        Text = 'Deny'
        Items.Strings = (
          'Deny'
          'Allow'
          'Pretend')
      end
      object DeleteModeComboBox: TComboBox
        Left = 72
        Top = 70
        Width = 105
        Height = 21
        Style = csDropDownList
        ItemIndex = 0
        TabOrder = 2
        Text = 'Deny'
        Items.Strings = (
          'Deny'
          'Allow'
          'Pretend')
      end
    end
    object DataGroupBox: TGroupBox
      Left = 1
      Top = 105
      Width = 331
      Height = 83
      Align = alTop
      Caption = 'Data'
      TabOrder = 1
      object ValueDataRichEdit: TRichEdit
        Left = 2
        Top = 15
        Width = 327
        Height = 66
        Align = alClient
        Font.Charset = EASTEUROPE_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'Tahoma'
        Font.Style = []
        ParentFont = False
        PlainText = True
        TabOrder = 0
      end
    end
    object BasicInformationGroupBox: TGroupBox
      Left = 1
      Top = 1
      Width = 331
      Height = 104
      Align = alTop
      Caption = 'Basic information'
      TabOrder = 2
      object Label1: TLabel
        Left = 11
        Top = 19
        Width = 47
        Height = 13
        Caption = 'Key name'
      end
      object Label2: TLabel
        Left = 11
        Top = 46
        Width = 55
        Height = 13
        Caption = 'Value name'
      end
      object Label3: TLabel
        Left = 11
        Top = 70
        Width = 24
        Height = 13
        Caption = 'Type'
      end
      object KeyNameEdit: TEdit
        Left = 72
        Top = 16
        Width = 256
        Height = 21
        TabOrder = 0
      end
      object ValueNameEdit: TEdit
        Left = 72
        Top = 43
        Width = 256
        Height = 21
        TabOrder = 1
      end
      object ValueTypeComboBox: TComboBox
        Left = 72
        Top = 70
        Width = 105
        Height = 21
        Style = csDropDownList
        ItemIndex = 1
        TabOrder = 2
        Text = 'REG_BINARY'
        Items.Strings = (
          'REG_NONE'
          'REG_BINARY'
          'REG_DWORD'
          'REG_QWORD'
          'REG_SZ'
          'REG_EXPAND_SZ'
          'REG_MULTI_SZ')
      end
    end
  end
  object OkButton: TButton
    Left = 192
    Top = 312
    Width = 65
    Height = 25
    Caption = 'Ok'
    TabOrder = 1
    OnClick = OkButtonClick
  end
  object StornoButton: TButton
    Left = 260
    Top = 312
    Width = 65
    Height = 25
    Caption = 'Storno'
    TabOrder = 2
    OnClick = StornoButtonClick
  end
end
