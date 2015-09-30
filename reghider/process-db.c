
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "hash_table.h"
#include "process-db.h"

/************************************************************************/
/*                           GLOBAL VARIABLES                           */
/************************************************************************/

static PHASH_TABLE _processTable = NULL;
static ERESOURCE _processTableLock;

/************************************************************************/
/*                           HELPER FUNCTIONS                           */
/************************************************************************/

static ULONG32 _HashFunction(PVOID Key)
{
	return ((ULONG32)Key >> 2);
}

static BOOLEAN _CompareFunction(PHASH_ITEM Item, PVOID Key)
{
	PPROCESSDB_PROCESS_RECORD rec = CONTAINING_RECORD(Item, PROCESSDB_PROCESS_RECORD, HashItem);

	return (rec->ProcessId == Key);
}

static VOID _FreeFunction(PHASH_ITEM Item, PVOID FreeContext)
{
	PPROCESSDB_PROCESS_RECORD rec = CONTAINING_RECORD(Item, PROCESSDB_PROCESS_RECORD, HashItem);
	DEBUG_ENTER_FUNCTION("Item=0x%p; FreeContext=0x%p", Item, FreeContext);

	ProcessDBRecordDereference(rec);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

static VOID _ProcessNotifyEx(_Inout_ PEPROCESS Process, _In_ HANDLE ProcessId, _In_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
	PPROCESSDB_PROCESS_RECORD rec = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Process=0x%p; ProcessId=0x%p; CreateInfo=0x%p", Process, ProcessId, CreateInfo);

	if (CreateInfo != NULL) {
		USHORT imageFileNameLen = (CreateInfo->ImageFileName != NULL) ? CreateInfo->ImageFileName->Length : 0;
	
		rec = (PPROCESSDB_PROCESS_RECORD)HeapMemoryAllocPaged(sizeof(PROCESSDB_PROCESS_RECORD) + imageFileNameLen);
		if (rec != NULL) {
			rec->ReferenceCount = 1;
			rec->ProcessId = ProcessId;
			rec->ParentId = CreateInfo->ParentProcessId;
			memset(&rec->ImageFileName, 0, sizeof(UNICODE_STRING));
			if (imageFileNameLen > 0) {
				rec->ImageFileName.Buffer = (PWCH)(rec + 1);
				rec->ImageFileName.Length = CreateInfo->ImageFileName->Length;
				rec->ImageFileName.MaximumLength = rec->ImageFileName.Length;
				memcpy(rec->ImageFileName.Buffer, CreateInfo->ImageFileName->Buffer, rec->ImageFileName.Length);
			}

			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&_processTableLock, TRUE);
			HashTableInsert(_processTable, &rec->HashItem, ProcessId);
			ExReleaseResourceLite(&_processTableLock);
			KeLeaveCriticalRegion();
			status = STATUS_SUCCESS;
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else {
		PHASH_ITEM h = NULL;

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&_processTableLock, TRUE);
		h = HashTableDelete(_processTable, ProcessId);
		ExReleaseResourceLite(&_processTableLock);
		KeLeaveCriticalRegion();
		if (h != NULL) {
			rec = CONTAINING_RECORD(h, PROCESSDB_PROCESS_RECORD, HashItem);
			ProcessDBRecordDereference(rec);
		}
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                          PUBLIC FUNCTIONS                            */
/************************************************************************/

NTSTATUS ProcessDBRecordGet(_In_ HANDLE ProcessId, _Out_ PPROCESSDB_PROCESS_RECORD *Record)
{
	PHASH_ITEM h = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PPROCESSDB_PROCESS_RECORD tmpRecord = NULL;
	DEBUG_ENTER_FUNCTION("ProcessId=0x%p; Record=0x%p", ProcessId, Record);

	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&_processTableLock, TRUE);
	h = HashTableGet(_processTable, ProcessId);
	if (h != NULL) {
		tmpRecord = CONTAINING_RECORD(h, PROCESSDB_PROCESS_RECORD, HashItem);
		InterlockedIncrement(&tmpRecord->ReferenceCount);
		*Record = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_NOT_FOUND;

	ExReleaseResourceLite(&_processTableLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}

VOID ProcessDBRecordDereference(_Inout_ PPROCESSDB_PROCESS_RECORD Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	if (InterlockedDecrement(&Record->ReferenceCount) == 0)
		HeapMemoryFree(Record);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/************************************************************************/
/*                          INITIALIZATION AND FINALIZATION             */
/************************************************************************/

NTSTATUS ProcessDBModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"", DriverObject, RegistryPath);

	status = ExInitializeResourceLite(&_processTableLock);
	if (NT_SUCCESS(status)) {
		status = HashTableCreate(httNoSynchronization, 37, _HashFunction, _CompareFunction, _FreeFunction, &_processTable);
		if (NT_SUCCESS(status)) {
			status = PsSetCreateProcessNotifyRoutineEx(_ProcessNotifyEx, FALSE);
			if (!NT_SUCCESS(status))
				HashTableDestroy(_processTable);
		}

		if (!NT_SUCCESS(status))
			ExDeleteResourceLite(&_processTableLock);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID ProcessDBModuleFinit(_In_ PDRIVER_OBJECT DriverObject)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	PsSetCreateProcessNotifyRoutineEx(_ProcessNotifyEx, TRUE);
	HashTableDestroy(_processTable);
	ExDeleteResourceLite(&_processTableLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
