
#ifndef __REGHIDER_TYPES_H__
#define __REGHIDER_TYPES_H__



/** Lists possible behaviors of the driver when someone attempts to change or delete a registry value
 *  the driver emulates. */
typedef enum _ERegistryValueOpMode {
	/** Deny the operation (the initiator gets STATUS_ACCESS_DENIED). */
	rvdmDeny,
	/** Permit the operation (the emulated value's characteristics change, or it is completely deleted (its emulation stops)). */
	rvdmAllow,
	/** Pretend that the operaton succeeded (The initiator gets STATUS_SUCCESS but nothing actually happens to the emulated value)). */
	rvdmPretend,
} ERegistryValueOpMode, *PERegistryValueOpMode;





#endif 
