
#ifndef __IOCTL_H__
#define __IOCTL_H__




#include "reghider-types.h"


#define REGHIDER_USERMODE_NAME							L"\\\\.\\RegHider"

#define IOCTL_REGHIDER_HIDDEN_KEY_ADD                  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x01, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_REGHIDER_HIDDEN_KEY_ENUM                 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x02, METHOD_NEITHER, FILE_READ_ACCESS)
#define IOCTL_REGHIDER_HIDDEN_KEY_DELETE               CTL_CODE(FILE_DEVICE_UNKNOWN, 0x03, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_REGHIDER_PSEUDO_VALUE_ADD                CTL_CODE(FILE_DEVICE_UNKNOWN, 0x11, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_REGHIDER_PSEUDO_VALUE_ENUM               CTL_CODE(FILE_DEVICE_UNKNOWN, 0x12, METHOD_NEITHER, FILE_READ_ACCESS)
#define IOCTL_REGHIDER_PSEUDO_VALUE_DELETE             CTL_CODE(FILE_DEVICE_UNKNOWN, 0x13, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_REGHIDER_PSUEDO_VALUE_SET				   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x14, METHOD_BUFFERED, FILE_WRITE_ACCESS)


/************************************************************************/
/*                     IOCTL_REGHIDER_HIDDEN_KEY_ADD                    */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT{
	USHORT KeyNameLength;
	WCHAR KeyName[1];
} IOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT, *PIOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT;

/************************************************************************/
/*                    IOCTL_REGHIDER_HIDDEN_KEY_ENUM                    */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT {
	ULONG NextEntryOffset;
	USHORT KeyNameLength;
	WCHAR KeyName[1];
} IOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT, *PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT;

/************************************************************************/
/*                   IOCTL_REGHIDER_HIDDEN_KEY_DELETE                   */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT {
	USHORT KeyNameLength;
	WCHAR KeyName[1];
} IOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT, *PIOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT;

/************************************************************************/
/*                 IOCTL_REGHIDER_PSEUDO_VALUE_ADD                      */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT {
	ULONG ValueType;
	ERegistryValueOpMode ChangeMode;
	ERegistryValueOpMode DeleteMode;
	ULONG DataLength;
	ULONG DataOffset;
	USHORT NameLength;
	ULONG NameOffset;
	USHORT ProcessNameLength;
	ULONG ProcessNameOffset;
	USHORT KeyNameLength;
	ULONG KeyNameOffset;
} IOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT, *PIOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT;

/************************************************************************/
/*                IOCTL_REGHIDER_PSEUDO_VALUE_ENUM                      */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT {
	ULONG NextEntryOffset;
	ULONG Valuetype;
	ERegistryValueOpMode ChangeMode;
	ERegistryValueOpMode DeleteMode;
	ULONG DataLength;
	ULONG DataOffset;
	USHORT NameLength;
	ULONG NameOffset;
	USHORT ProcessNameLength;
	ULONG ProcessNameOffset;
	USHORT KeyNameLength;
	ULONG KeyNameOffset;
} IOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT, *PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT;

/************************************************************************/
/*              IOCTL_REGHIDER_PSEUDO_VALUE_DELETE                      */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT {
	USHORT KeyNameLength;
	ULONG KeyNameOffset;
	USHORT ValueNameLength;
	ULONG ValueNameOffset;
} IOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT, *PIOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT;

/************************************************************************/
/*                 IOCTL_REGHIDER_PSUEDO_VALUE_SET                      */
/************************************************************************/

typedef struct _IOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT {
	ULONG KeyNameOffset;
	ULONG ValueNameOffset;
	ULONG ProcessNameOffset;
	ULONG DataOffset;
	ULONG DataLength;
	ULONG ValueType;
	ERegistryValueOpMode ChangeMode;
	ERegistryValueOpMode DeleteMode;
	USHORT KeyNameLength;
	USHORT ValeNameLength;
	USHORT ProcessNameLength;
} IOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT, *PIOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT;



#endif 
