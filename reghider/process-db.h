
#ifndef __PROCESS_DB_H__
#define __PROCESS_DB_H__

#include <ntifs.h>
#include "hash_table.h"


typedef struct _PROCESSDB_PROCESS_RECORD {
	HASH_ITEM HashItem;
	volatile LONG ReferenceCount;
	HANDLE ProcessId;
	HANDLE ParentId;
	UNICODE_STRING ImageFileName;
} PROCESSDB_PROCESS_RECORD, *PPROCESSDB_PROCESS_RECORD;



NTSTATUS ProcessDBRecordGet(_In_ HANDLE ProcessId, _Out_ PPROCESSDB_PROCESS_RECORD *Record);
VOID ProcessDBRecordDereference(_Inout_ PPROCESSDB_PROCESS_RECORD Record);

NTSTATUS ProcessDBModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID ProcessDBModuleFinit(_In_ PDRIVER_OBJECT DriverObject);



#endif 
