
#ifndef __LIBREGHIDER_H__
#define __LIBREGHIDER_H__


#include <windows.h>
#include "reghider-types.h"
#include "libreghider-types.h"


/** @abstract 
 *  Hides a registry key.
 *
 *  @description
 *  Adds a new entry to the list of keys that will be hidden by the driver.
 *
 *  The key name given in the only function argument must be an absolute path to the target
 *  registry key. That menas, it must start with the \Registry prefix. The driver does not check
 *  whether the given path is valid.
 *
 *  @param KeyName A full name of the key to hide.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS menas success).
 *
 *  @remark
 *  The absolute name of the key may be used in calls to other library functions manipulating with
 *  hidden registry keys.
 */
DWORD LibRegHiderHiddenKeyAdd(PWCHAR KeyName);

/** @abstract
 *  Deletes a given name from the list of registry keys hidden by the driver.
 *
 *  @param KeyName The name to delete. The name must match the one specified when adding the registry
 *  key to the list.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS menas success).
 */
DWORD LibRegHiderHiddenKeyDelete(PWCHAR KeyName);

/** Enumerates all registry keys being hidden by the driver.
 *
 *  @param Callback Address of a callback function that is invoked by the library with information about
 *  each single registry key hidden by the driver.
 *  @param Context User-defined value that is passed to each call of the callback.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS menas success).
 */
DWORD LibRegHiderHiddenKeysEnum(REGHIDER_HIDDEN_KEY_CALLBACK *Callback, PVOID Context);

/** @abstract
 *  Instructs the driver to emulate a registry value.
 *
 *  @param KeyName Full name of a registry key for which the value will be reported. The name must start with the
 *  \Registry prefix.
 *  @param ValueName Name of the value. If the key already has a value of such name, both values (the emulated one and the real one)
 *  will be reported (the emulated one will be reported first) which might not be a good things to happen.
 *  @param ValueType Type of the value data.
 *  @param Data Buffer with value data.
 *  @param DataLength Length of the value data buffer.
 *  @param DeleteMode Determines what the driver should do when someone attempts to delete the emulated value.
 *  @param ChangeMode Determines what the driver should do when someone attempts to change value's characteristics.
 *  @param ProcessName A string. If non-NULL, the value will be presented only to processes the image file name of which
 *  ends to this string.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS menas success).
 */
DWORD LibRegHiderPseudoValueAdd(PWCHAR KeyName, PWCHAR ValueName, ULONG Valuetype, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName);

/** @abstract
 *  Instructs the driver to stop emulating a given value.
 *
 *  @param KeyName Full name of the key on which the value is emulated.
 *  @param ValueName Name of the value.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS menas success).
 */
DWORD LibRegHiderPseudoValueDelete(PWCHAR KeyName, PWCHAR ValueName);

/** @abstract
 *  Enumerates all registry values emulated by the driver.
 *
 *  @param Callback Address of a callback routine that will be called to provide the user with
 *  information about each value emulated by the driver.
 *  @param Context User-defined value passed to every call of the callback.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS menas success).
 */
DWORD LibRegHiderPseudoValuesEnum(REGHIDER_PSEUDO_VALUE_CALLBACK *Callback, PVOID Context);

/** @abstract 
 *  Changes settings for a given emulated value.
 *
 *  @description
 *  The routine allows to change value type, data, change and delete modes and a process name. It is
 *  not possible to change key or value name by this routine.
 *
 *  @param KeyName Full name of the key.
 *  @param ValueName Name of the emulated value.
 *  @param ValueType Type of the value data.
 *  @param Data Buffer with value data.
 *  @param DataLength Length of the value data buffer.
 *  @param DeleteMode Determines what the driver should do when someone attempts to delete the emulated value.
 *  @param ChangeMode Determines what the driver should do when someone attempts to change value's characteristics.
 *  @param ProcessName A string. If non-NULL, the value will be presented only to processes the image file name of which
 *  ends to this string.
 *
 *  @return
 *  ERROR_XXX (ERROR_SUCCESS for success).
 */
DWORD LibRegHiderPseudoValueSet(PWCHAR KeyName, PWCHAR ValueName, ULONG ValueType, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName);


/** @abstract
 *  Initializes the reghider.dll library. 
 *
 *  @description
 *  The routine just attempts to get a handle to driver's device. It neither
 *  installs, nor loads the driver. You must do these steps yourself.
 *
 *  @return
 *    @value ERROR_SUCCESS Initialization succeeded.
 *   @value Other An error occurred.
 */
DWORD LibRegHiderInit(VOID);

/** @abstract
 *  Finalizes the library.
 *
 *  @description
 *  Just disconnects driver's device (closes the handle to it).
 */
VOID LibRegHiderFinit(VOID);



#endif 
