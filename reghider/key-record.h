
#ifndef __KEY_RECORD_H__
#define __KEY_RECORD_H__

#include <ntifs.h>
#include "reghider-types.h"
#include "hash_table.h"
#include "string-hash-table.h"



typedef struct _REGISTRY_VALUE_RECORD {
	LIST_ENTRY Entry;
	volatile LONG ReferenceCount;
	BOOLEAN DeletePending;
	UNICODE_STRING ValueName;
	ERESOURCE DataLock;
	ULONG ValueType;
	ULONG DataLength;
	PVOID Data;
	ERESOURCE ProcessNameLock;
	UNICODE_STRING ProcessName;
	ERESOURCE ModeLock;
	ERegistryValueOpMode DeleteMode;
	ERegistryValueOpMode ChangeMode;
} REGISTRY_VALUE_RECORD, *PREGISTRY_VALUE_RECORD;


typedef struct _REGISTRY_SUBKEY_RECORD {
	LIST_ENTRY Entry;
	volatile LONG ReferenceCount;
	BOOLEAN DeletePending;
	UNICODE_STRING Name;
	UNICODE_STRING ProcessName;
} REGISTRY_SUBKEY_RECORD, *PREGISTRY_SUBKEY_RECORD;

typedef struct _REGISTRY_KEY_RECORD {
	/** Links all key records together to make their enumeration more easy. */
	LIST_ENTRY Entry;
	/** Number of references pointing to the record. If it drops to zero, the record
	    is automatically freed. */
	volatile LONG ReferenceCount;
	BOOLEAN DeletePending;
	UNICODE_STRING Name;
	/** Contains subkeys that will be hidden. 
	    Key = name of the subkey
		data = REGISTRY_SUBKEY_RECORD */
	PHASH_TABLE HiddenSubkeys;
	LIST_ENTRY HiddenSubkeyListHead;
	ERESOURCE HiddenSubkeyLock;
	/** Contains values that will be emulated by the driver. 
	    key = value name
		data = REGISTRY_VALUE_RECORD */
	PHASH_TABLE PseudoValues;
	LIST_ENTRY PseudoValueListHead;
	ERESOURCE PseudoValueLock;
} REGISTRY_KEY_RECORD, *PREGISTRY_KEY_RECORD;




NTSTATUS KeyRecordCreate(_In_ PUNICODE_STRING KeyName, _Out_ PREGISTRY_KEY_RECORD *Record);
NTSTATUS KeyRecordDelete(_In_ PUNICODE_STRING KeyName);
VOID KeyRecordDereference(_Inout_ PREGISTRY_KEY_RECORD Record);
NTSTATUS KeyRecordGet(_In_ PUNICODE_STRING KeyName, _Out_ PREGISTRY_KEY_RECORD *Record);
NTSTATUS KeyRecordsEnumerate(_Out_ PREGISTRY_KEY_RECORD **Array, _Out_ PSIZE_T Count);
VOID KeyRecordsDereference(_Inout_ PREGISTRY_KEY_RECORD *Array, _In_ SIZE_T Count);

NTSTATUS KeyRecordAddHiddenSubkey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING SubkyName, _In_opt_ PUNICODE_STRING ProcessName);
NTSTATUS KeyRecordDeleteHiddenSubkey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING SubkeyName);
NTSTATUS KeyRecordGetHiddenSubkey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING SubkeyName, _Out_ PREGISTRY_SUBKEY_RECORD *SubkeyRecord);
VOID KeyRecordHiddenSubkeyDereference(_In_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_SUBKEY_RECORD SubkeyRecord);
NTSTATUS KeyRecordHiddenSubkeysEnumerate(_In_ PREGISTRY_KEY_RECORD Record, _Out_ PREGISTRY_SUBKEY_RECORD **Array, _Out_ PSIZE_T Count);
VOID KeyRecordHiddenSubkeysDereference(_Inout_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_SUBKEY_RECORD *Array, _In_ SIZE_T Count);

NTSTATUS KeyRecordAddPseudoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName, _In_ ULONG ValueType, _In_opt_ ULONG ValueDataLength, _In_opt_ PVOID ValueData, _In_ ERegistryValueOpMode DeleteMode, _In_ ERegistryValueOpMode ChangeMode, _In_opt_ PUNICODE_STRING ProcessName);
NTSTATUS KeyRecordChangePseudoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREGISTRY_VALUE_RECORD ValueRecord, _In_ ULONG ValueType, _In_opt_ PVOID Data, _In_opt_ ULONG DataLength, _In_ ERegistryValueOpMode ChangeMode, _In_ ERegistryValueOpMode DeleteMode, _In_opt_ PUNICODE_STRING ProcessName);
NTSTATUS KeyRecordDeletePsuedoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName);
NTSTATUS KeyRecordGetPseudoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName, _Out_ PREGISTRY_VALUE_RECORD *ValueRecord);
NTSTATUS KeyRecordGetPseudoValueByIndex(_In_ PREGISTRY_KEY_RECORD Record, _In_ ULONG Index, _Out_ PREGISTRY_VALUE_RECORD *ValueRecord);
ULONG KeyRecordGetPseudoValuesCount(_In_ PREGISTRY_KEY_RECORD Record);
VOID KeyRecordPseudoValueDereference(_In_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_VALUE_RECORD ValueRecord);
NTSTATUS KeyRecordPseudoValuesEnumerate(_In_ PREGISTRY_KEY_RECORD Record, _Out_ PREGISTRY_VALUE_RECORD **Array, _Out_ PSIZE_T Count);
VOID KeyRecordPseudoValuesDereference(_Inout_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_VALUE_RECORD *Array, _In_ SIZE_T Count);


NTSTATUS KeyRecordModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID KeyRecordModuleFInit(_In_ PDRIVER_OBJECT DriverObject);



#endif 
