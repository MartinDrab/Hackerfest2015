object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'Registry Hider'
  ClientHeight = 362
  ClientWidth = 591
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'Tahoma'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object HiddenRegistryKeysGroupBox: TGroupBox
    Left = 0
    Top = 0
    Width = 591
    Height = 169
    Align = alTop
    Anchors = [akLeft, akTop, akRight, akBottom]
    Caption = 'Hidden registry keys'
    TabOrder = 0
    object HiddenRegistryKeysPanel: TPanel
      Left = 2
      Top = 134
      Width = 587
      Height = 33
      Align = alBottom
      TabOrder = 0
      object HiddenKeysAddButton: TButton
        Left = 8
        Top = 8
        Width = 57
        Height = 21
        Caption = 'Add...'
        TabOrder = 0
        OnClick = AddButtonClick
      end
      object HiddenKeysDeleteButton: TButton
        Left = 71
        Top = 8
        Width = 57
        Height = 21
        Caption = 'Delete'
        TabOrder = 1
        OnClick = DeleteButtonClick
      end
      object HiddenKeysRefreshButton: TButton
        Left = 134
        Top = 8
        Width = 57
        Height = 21
        Caption = 'Refresh'
        TabOrder = 2
        OnClick = RefreshButtonClick
      end
    end
    object HiddenKeysListView: TListView
      Left = 2
      Top = 15
      Width = 587
      Height = 119
      Align = alClient
      Columns = <
        item
          AutoSize = True
          Caption = 'Name'
        end>
      OwnerData = True
      ReadOnly = True
      RowSelect = True
      ShowWorkAreas = True
      TabOrder = 1
      ViewStyle = vsReport
      OnData = ListViewData
    end
  end
  object PseudoRegistryValuesGroupBox: TGroupBox
    Left = 0
    Top = 169
    Width = 591
    Height = 193
    Align = alBottom
    Anchors = [akLeft, akTop, akRight, akBottom]
    Caption = 'Registry pseudo values'
    TabOrder = 1
    object PseudoRegistryValuesPanel: TPanel
      Left = 2
      Top = 158
      Width = 587
      Height = 33
      Align = alBottom
      TabOrder = 0
      ExplicitTop = 149
      object PseudoValuesAddButton: TButton
        Left = 8
        Top = 6
        Width = 57
        Height = 21
        Caption = 'Add...'
        TabOrder = 0
        OnClick = AddButtonClick
      end
      object PseudoValuesDeleteButton: TButton
        Left = 127
        Top = 6
        Width = 57
        Height = 21
        Caption = 'Delete'
        TabOrder = 1
        OnClick = DeleteButtonClick
      end
      object PseudoValuesRefreshButton: TButton
        Left = 190
        Top = 6
        Width = 57
        Height = 21
        Caption = 'Refresh'
        TabOrder = 2
        OnClick = RefreshButtonClick
      end
      object PseudoValueEditButton: TButton
        Left = 71
        Top = 6
        Width = 57
        Height = 21
        Caption = 'Edit...'
        TabOrder = 3
        OnClick = AddButtonClick
      end
    end
    object PseudoValuesListView: TListView
      Left = 2
      Top = 15
      Width = 587
      Height = 143
      Align = alClient
      Columns = <
        item
          AutoSize = True
          Caption = 'Key'
        end
        item
          Caption = 'Value'
          Width = 100
        end
        item
          Caption = 'Type'
          Width = 100
        end
        item
          Caption = 'Modes'
          Width = 100
        end
        item
          AutoSize = True
          Caption = 'Data'
        end
        item
          AutoSize = True
          Caption = 'Process'
        end>
      OwnerData = True
      ReadOnly = True
      RowSelect = True
      ShowWorkAreas = True
      TabOrder = 1
      ViewStyle = vsReport
      OnData = ListViewData
      ExplicitHeight = 134
    end
  end
end
