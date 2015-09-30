
#include <windows.h>
#include "reghider-types.h"
#include "libreghider-types.h"
#include "libreghider.h"
#include "dllreghider.h"

/************************************************************************/
/*                    EXPORTED FUNCTIONS                                */
/************************************************************************/

/**************/
/* HIDDEN KEY */
/**************/

REGHIDER_API DWORD WINAPI HiddenKeyAdd(PWCHAR KeyName)
{
	return LibRegHiderHiddenKeyAdd(KeyName);
}

REGHIDER_API DWORD WINAPI HiddenKeyDelete(PWCHAR KeyName)
{
	return LibRegHiderHiddenKeyDelete(KeyName);
}

REGHIDER_API DWORD WINAPI HiddenKeysEnum(REGHIDER_HIDDEN_KEY_CALLBACK *Callback, PVOID Context)
{
	return LibRegHiderHiddenKeysEnum(Callback, Context);
}

/****************/
/* PSEUDO VALUE */
/****************/

REGHIDER_API DWORD WINAPI PseudoValueAdd(PWCHAR KeyName, PWCHAR ValueName, ULONG Valuetype, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName)
{
	return LibRegHiderPseudoValueAdd(KeyName, ValueName, Valuetype, Data, DataLength, DeleteMode, ChangeMode, ProcessName);
}

REGHIDER_API DWORD WINAPI PseudoValueDelete(PWCHAR KeyName, PWCHAR ValueName)
{
	return LibRegHiderPseudoValueDelete(KeyName, ValueName);
}

REGHIDER_API DWORD WINAPI PseudoValuesEnum(REGHIDER_PSEUDO_VALUE_CALLBACK *Callback, PVOID Context)
{
	return LibRegHiderPseudoValuesEnum(Callback, Context);
}

REGHIDER_API DWORD WINAPI PseudoValueSet(PWCHAR KeyName, PWCHAR ValueName, ULONG ValueType, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName)
{
	return LibRegHiderPseudoValueSet(KeyName, ValueName, ValueType, Data, DataLength, DeleteMode, ChangeMode, ProcessName);
}


/******************/
/* INIT AND FINIT */
/******************/

REGHIDER_API DWORD WINAPI Init(VOID)
{
	return LibRegHiderInit();
}

REGHIDER_API VOID WINAPI Finit(VOID)
{
	LibRegHiderFinit();

	return;
}



/************************************************************************/
/*                   INITIALIZATION AND FINALIZATION                    */
/************************************************************************/

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, PVOID lpReserved)
{
	BOOL ret = FALSE;

	switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			ret = DisableThreadLibraryCalls(hInstance);
			break;
	}

	return ret;
}
