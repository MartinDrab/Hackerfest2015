
#ifndef __DLLREGHIDER_H__
#define __DLLREGHIDER_H__

#include <windows.h>
#include "reghider-types.h"
#include "libreghider-types.h"

#ifdef DLLREGHIDER_EXPORTS

#define REGHIDER_API								EXTERN_C __declspec(dllexport)

#else 

#define REGHIDER_API								EXTERN_C __declspec(dllimport)

#endif



REGHIDER_API DWORD WINAPI HiddenKeyAdd(PWCHAR KeyName);
REGHIDER_API DWORD WINAPI HiddenKeyDelete(PWCHAR KeyName);
REGHIDER_API DWORD WINAPI HiddenKeysEnum(REGHIDER_HIDDEN_KEY_CALLBACK *Callback, PVOID Context);

REGHIDER_API DWORD WINAPI PseudoValueAdd(PWCHAR KeyName, PWCHAR ValueName, ULONG Valuetype, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName);
REGHIDER_API DWORD WINAPI PseudoValueDelete(PWCHAR KeyName, PWCHAR ValueName);
REGHIDER_API DWORD WINAPI PseudoValuesEnum(REGHIDER_PSEUDO_VALUE_CALLBACK *Callback, PVOID Context);
REGHIDER_API DWORD WINAPI PseudoValueSet(PWCHAR KeyName, PWCHAR ValueName, ULONG ValueType, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName);

REGHIDER_API DWORD WINAPI Init(VOID);
REGHIDER_API VOID WINAPI Finit(VOID);




#endif 
