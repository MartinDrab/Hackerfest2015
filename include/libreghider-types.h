
#ifndef __LIBREGHIDER_TYPES_H__
#define __LIBREGHIDER_TYPES_H__

#include <windows.h>
#include "reghider-types.h"



/** Stores information about one registry key hidden by the driver. */
typedef struct _REGHIDER_HIDDEN_KEY_RECORD {
	/** Full name of the key as it was specified when adding it to the list of hidden keys. */
	PWCHAR KeyName;
} REGHIDER_HIDDEN_KEY_RECORD, *PREGHIDER_HIDDEN_KEY_RECORD;

/** Stores information about one registry value that is emulated by the driver. */
typedef struct _REGHIDER_PSEUDO_VALUE_RECORD {
	/** Full name of a registry key to which the value belongs. */
	PWCHAR KeyName;
	/** Name of the value. */
	PWCHAR ValueName;
	/** Type of the value data (REG_XXX). */
	ULONG ValueType;
	/** Address of the value data buffer. */
	PVOID Data;
	/** Length of the value data. */
	ULONG DataLength;
	/** Determines what the driver should do when someone attempts to change value characteristics (SetValue operation). */
	ERegistryValueOpMode ChangeMode;
	/** Determines what the driver should do when someone attempts to delete the value (DeleteValue operation). */
	ERegistryValueOpMode DeleteMode;
	/** A suffix that determines which processes are able to see the value. Only processes with image file name ending
	    with the suffix can see the value. */
	PWCHAR ProcessName;
} REGHIDER_PSEUDO_VALUE_RECORD, *PREGHIDER_PSEUDO_VALUE_RECORD;


/** The callback is used to report one entry in the list of hidden registry keys.
 *
 *  @param Record Address of the record with information about the key.
 *  @param Context User-defined value passed to the @link(LibRegHiderHiddenKeysEnum)
 *  as the second argument.
 *
 *  @return
 *    @value FALSE Abort the enumeration.
 *    @value TRUE Continue with the enumeration.
 */
typedef BOOL(WINAPI REGHIDER_HIDDEN_KEY_CALLBACK)(PREGHIDER_HIDDEN_KEY_RECORD Record, PVOID Context);

/** The callback is used to report one entry in the list of registry values emulated by the driver.
*
*  @param Record Address of the record with information about the value.
*  @param Context User-defined value passed to the @link(LibRegHiderPseudoValuesEnum)
*  as the second argument.
*
*  @return
*    @value FALSE Abort the enumeration.
*    @value TRUE Continue with the enumeration.
*/
typedef BOOL(WINAPI REGHIDER_PSEUDO_VALUE_CALLBACK)(PREGHIDER_PSEUDO_VALUE_RECORD Record, PVOID Context);




#endif 
