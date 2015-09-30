
#include <ntifs.h>
#include "allocator.h"
#include "preprocessor.h"
#include "utils-dym-array.h"
#include "hash_table.h"
#include "string-hash-table.h"
#include "process-db.h"
#include "key-record.h"


/************************************************************************/
/*                       GLOBAL VARIABLES                               */
/************************************************************************/

static LIST_ENTRY _keyRecordListHead;
static PHASH_TABLE _keyRecordTable;
static ERESOURCE _keyRecordLock;

/************************************************************************/
/*                       HELPER FUNCTIONS                               */
/************************************************************************/



/************************************************************************/
/*                       PUBLIC FUNCTIONS                               */
/************************************************************************/


NTSTATUS KeyRecordCreate(_In_ PUNICODE_STRING KeyName, _Out_ PREGISTRY_KEY_RECORD *Record)
{
	PREGISTRY_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyName=\"%wZ\"; Record=0x%p", KeyName, Record);

	tmpRecord = (PREGISTRY_KEY_RECORD)HeapMemoryAllocPaged(sizeof(REGISTRY_KEY_RECORD) + KeyName->Length);
	if (tmpRecord != NULL) {
		InitializeListHead(&tmpRecord->Entry);
		tmpRecord->ReferenceCount = 1;
		tmpRecord->DeletePending = FALSE;
		tmpRecord->Name = *KeyName;
		tmpRecord->Name.Buffer = (PWCH)(tmpRecord + 1);
		memcpy(tmpRecord->Name.Buffer, KeyName->Buffer, tmpRecord->Name.Length);
		status = StringHashTableCreate(httNoSynchronization, 37, &tmpRecord->HiddenSubkeys);
		if (NT_SUCCESS(status)) {
			InitializeListHead(&tmpRecord->HiddenSubkeyListHead);
			status = ExInitializeResourceLite(&tmpRecord->HiddenSubkeyLock);
			if (NT_SUCCESS(status)) {
				status = StringHashTableCreate(httNoSynchronization, 37, &tmpRecord->PseudoValues);
				if (NT_SUCCESS(status)) {
					InitializeListHead(&tmpRecord->PseudoValueListHead);
					status = ExInitializeResourceLite(&tmpRecord->PseudoValueLock);
					if (NT_SUCCESS(status)) {
						KeEnterCriticalRegion();
						ExAcquireResourceExclusiveLite(&_keyRecordLock, TRUE);
						status = stringHashTableInsertUnicodeString(_keyRecordTable, &tmpRecord->Name, tmpRecord);
						if (NT_SUCCESS(status)) {
							InterlockedIncrement(&tmpRecord->ReferenceCount);
							InsertTailList(&_keyRecordListHead, &tmpRecord->Entry);
							*Record = tmpRecord;
						}

						ExReleaseResourceLite(&_keyRecordLock);
						KeLeaveCriticalRegion();
						if (!NT_SUCCESS(status))
							ExDeleteResourceLite(&tmpRecord->PseudoValueLock);
					}

					if (!NT_SUCCESS(status))
						StringHashTableDestroy(tmpRecord->PseudoValues);
				}

				if (!NT_SUCCESS(status))
					ExDeleteResourceLite(&tmpRecord->HiddenSubkeyLock);
			}

			if (!NT_SUCCESS(status))
				StringHashTableDestroy(tmpRecord->HiddenSubkeys);
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}


NTSTATUS KeyRecordDelete(_In_ PUNICODE_STRING KeyName)
{
	PREGISTRY_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyName=\"%wZ\"", KeyName);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&_keyRecordLock, TRUE);
	tmpRecord = (PREGISTRY_KEY_RECORD)StringHashTableDeleteUnicodeString(_keyRecordTable, KeyName);
	ExReleaseResourceLite(&_keyRecordLock);
	KeLeaveCriticalRegion();
	if (tmpRecord != NULL) {
		tmpRecord->DeletePending = TRUE;
		KeyRecordDereference(tmpRecord);
		status = STATUS_SUCCESS;
	} else status = STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID KeyRecordDereference(_Inout_ PREGISTRY_KEY_RECORD Record)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	if (InterlockedDecrement(&Record->ReferenceCount) == 0) {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&_keyRecordLock, TRUE);
		RemoveEntryList(&Record->Entry);
		ExReleaseResourceLite(&_keyRecordLock);
		KeLeaveCriticalRegion();

		// TODO: Walk the hash table and dereference all items
		StringHashTableDestroy(Record->PseudoValues);
		ExDeleteResourceLite(&Record->PseudoValueLock);

		// TODO: Walk the hash table and dereference all items
		StringHashTableDestroy(Record->HiddenSubkeys);
		ExDeleteResourceLite(&Record->HiddenSubkeyLock);
		HeapMemoryFree(Record);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


NTSTATUS KeyRecordGet(_In_ PUNICODE_STRING KeyName, _Out_ PREGISTRY_KEY_RECORD *Record)
{
	PREGISTRY_KEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyName=\"%wZ\"; Record=0x%p", KeyName, Record);

	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&_keyRecordLock, TRUE);
	tmpRecord = (PREGISTRY_KEY_RECORD)StringHashTableGetUnicodeString(_keyRecordTable, KeyName);
	if (tmpRecord != NULL) {
		ASSERT(!tmpRecord->DeletePending);
		InterlockedIncrement(&tmpRecord->ReferenceCount);
		*Record = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_OBJECT_NAME_NOT_FOUND;

	ExReleaseResourceLite(&_keyRecordLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x, *Record=0x%p", status, *Record);
	return status;
}

NTSTATUS KeyRecordsEnumerate(_Out_ PREGISTRY_KEY_RECORD **Array, _Out_ PSIZE_T Count)
{
	SIZE_T tmpCount = 0;
	PREGISTRY_KEY_RECORD *tmpArray = NULL;
	ULONG recordCount = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=0x%p", Array, Count);

	status = STATUS_SUCCESS;
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&_keyRecordLock, TRUE);
	recordCount = StringHashTableGetItemCount(_keyRecordTable);
	if (recordCount > 0) {
		PUTILS_DYM_ARRAY dymArray = NULL;

		status = DymArrayCreate(PagedPool, &dymArray);
		if (NT_SUCCESS(status)) {
			status = DymArrayReserve(dymArray, recordCount);
			if (NT_SUCCESS(status)) {
				PREGISTRY_KEY_RECORD keyRecord = NULL;

				keyRecord = CONTAINING_RECORD(_keyRecordListHead.Flink, REGISTRY_KEY_RECORD, Entry);
				while (&keyRecord->Entry != &_keyRecordListHead) {
					if (!keyRecord->DeletePending) {
						InterlockedIncrement(&keyRecord->ReferenceCount);
						DymArrayPushBackNoAlloc(dymArray, keyRecord);
					}

					keyRecord = CONTAINING_RECORD(keyRecord->Entry.Flink, REGISTRY_KEY_RECORD, Entry);
				}

				tmpCount = DymArrayLength(dymArray);
				status = DymArrayToStaticArrayAlloc(dymArray, PagedPool, (PVOID **)&tmpArray);
				if (!NT_SUCCESS(status)) {
					ULONG i = 0;

					for (i = 0; i < tmpCount; ++i)
						KeyRecordDereference((PREGISTRY_KEY_RECORD)DymArrayItem(dymArray, i));
				}
			}

			DymArrayDestroy(dymArray);
		}
	} else status = STATUS_NO_MORE_ENTRIES;

	ExReleaseResourceLite(&_keyRecordLock);
	KeLeaveCriticalRegion();
	if (NT_SUCCESS(status)) {
		*Array = tmpArray;
		*Count = tmpCount;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Array=0x%p, *Count=%u", status, *Array, *Count);
	return status;
}

VOID KeyRecordsDereference(_Inout_ PREGISTRY_KEY_RECORD *Array, _In_ SIZE_T Count)
{
	DEBUG_ENTER_FUNCTION("Array=0x%p; Count=%u", Array, Count);

	if (Count > 0) {
		ULONG i = 0;

		for (i = 0; i < Count; ++i)
			KeyRecordDereference(Array[i]);

		HeapMemoryFree(Array);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


/******************/
/* HIDDEN SUBKEYS */
/******************/

NTSTATUS KeyRecordAddHiddenSubkey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING SubkyName, _In_opt_ PUNICODE_STRING ProcessName)
{
	USHORT processNameLen = 0;
	PREGISTRY_SUBKEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; SubkeyName=\"%wZ\"; ProcessName=\"%wZ\"", Record, SubkyName, ProcessName);

	processNameLen = (ProcessName != NULL) ? ProcessName->Length : 0;
	tmpRecord = (PREGISTRY_SUBKEY_RECORD)HeapMemoryAllocPaged(sizeof(REGISTRY_SUBKEY_RECORD) + SubkyName->Length + processNameLen);
	if (tmpRecord != NULL) {
		InitializeListHead(&tmpRecord->Entry);
		tmpRecord->Name = *SubkyName;
		tmpRecord->Name.Buffer = (PWCH)(tmpRecord + 1);
		memcpy(tmpRecord->Name.Buffer, SubkyName->Buffer, tmpRecord->Name.Length);
		tmpRecord->ReferenceCount = 1;
		tmpRecord->DeletePending = FALSE;
		memset(&tmpRecord->ProcessName, 0, sizeof(UNICODE_STRING));
		if (processNameLen > 0) {
			tmpRecord->ProcessName = *ProcessName;
			tmpRecord->ProcessName.Buffer = (PWCH)(tmpRecord + 1) + (SubkyName->Length / sizeof(WCHAR));
			memcpy(tmpRecord->ProcessName.Buffer, ProcessName->Buffer, tmpRecord->ProcessName.Length);
		}

		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->HiddenSubkeyLock, TRUE);
		status = stringHashTableInsertUnicodeString(Record->HiddenSubkeys, &tmpRecord->Name, tmpRecord);
		if (NT_SUCCESS(status)) {
			InterlockedIncrement(&tmpRecord->ReferenceCount);
			InsertTailList(&Record->HiddenSubkeyListHead, &tmpRecord->Entry);
			KeyRecordHiddenSubkeyDereference(Record, tmpRecord);
		}

		ExReleaseResourceLite(&Record->HiddenSubkeyLock);
		KeLeaveCriticalRegion();
		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_INSUFFICIENT_RESOURCES;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS KeyRecordDeleteHiddenSubkey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING SubkeyName)
{
	PREGISTRY_SUBKEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; SubkeyName=\"%wZ\"", Record, SubkeyName);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Record->HiddenSubkeyLock, TRUE);
	tmpRecord = (PREGISTRY_SUBKEY_RECORD)StringHashTableDeleteUnicodeString(Record->HiddenSubkeys, SubkeyName);
	ExReleaseResourceLite(&Record->HiddenSubkeyLock);
	KeLeaveCriticalRegion();
	if (tmpRecord != NULL) {
		tmpRecord->DeletePending = TRUE;
		KeyRecordHiddenSubkeyDereference(Record, tmpRecord);
		status = STATUS_SUCCESS;
	} else status = STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS KeyRecordGetHiddenSubkey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING SubkeyName, _Out_ PREGISTRY_SUBKEY_RECORD *SubkeyRecord)
{
	PREGISTRY_SUBKEY_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; SubkeyName=\"%wZ\"; SubkeyRecord=0x%p", Record, SubkeyName, SubkeyRecord);

	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&Record->HiddenSubkeyLock, TRUE);
	tmpRecord = (PREGISTRY_SUBKEY_RECORD)StringHashTableGetUnicodeString(Record->HiddenSubkeys, SubkeyName);
	if (tmpRecord != NULL) {
		ASSERT(!tmpRecord->DeletePending);
		InterlockedIncrement(&tmpRecord->ReferenceCount);
		*SubkeyRecord = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_OBJECT_NAME_NOT_FOUND;

	ExReleaseResourceLite(&Record->HiddenSubkeyLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x, *SubkeyRecord=0x%p", status, *SubkeyRecord);
	return status;
}


VOID KeyRecordHiddenSubkeyDereference(_In_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_SUBKEY_RECORD SubkeyRecord)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p; SubkeyRecord=0x%p", Record, SubkeyRecord);

	if (InterlockedDecrement(&SubkeyRecord->ReferenceCount) == 0) {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->HiddenSubkeyLock, TRUE);
		RemoveEntryList(&SubkeyRecord->Entry);
		ExReleaseResourceLite(&Record->HiddenSubkeyLock);
		KeLeaveCriticalRegion();
		HeapMemoryFree(SubkeyRecord);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS KeyRecordHiddenSubkeysEnumerate(_In_ PREGISTRY_KEY_RECORD Record, _Out_ PREGISTRY_SUBKEY_RECORD **Array, _Out_ PSIZE_T Count)
{
	SIZE_T tmpCount = 0;
	PREGISTRY_SUBKEY_RECORD *tmpArray = NULL;
	ULONG recordCount = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Array=0x%p; Count=0x%p", Record, Array, Count);

	status = STATUS_SUCCESS;
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&Record->HiddenSubkeyLock, TRUE);
	recordCount = StringHashTableGetItemCount(Record->HiddenSubkeys);
	if (recordCount > 0) {
		PUTILS_DYM_ARRAY dymArray = NULL;

		status = DymArrayCreate(PagedPool, &dymArray);
		if (NT_SUCCESS(status)) {
			status = DymArrayReserve(dymArray, recordCount);
			if (NT_SUCCESS(status)) {
				PREGISTRY_SUBKEY_RECORD subkeyRecord = NULL;

				subkeyRecord = CONTAINING_RECORD(Record->HiddenSubkeyListHead.Flink, REGISTRY_SUBKEY_RECORD, Entry);
				while (&subkeyRecord->Entry != &Record->HiddenSubkeyListHead) {
					if (!subkeyRecord->DeletePending) {
						InterlockedIncrement(&subkeyRecord->ReferenceCount);
						DymArrayPushBackNoAlloc(dymArray, subkeyRecord);
					}

					subkeyRecord = CONTAINING_RECORD(subkeyRecord->Entry.Flink, REGISTRY_SUBKEY_RECORD, Entry);
				}

				tmpCount = DymArrayLength(dymArray);
				status = DymArrayToStaticArrayAlloc(dymArray, PagedPool, (PVOID **)&tmpArray);
				if (!NT_SUCCESS(status)) {
					ULONG i = 0;

					for (i = 0; i < tmpCount; ++i)
						KeyRecordHiddenSubkeyDereference(Record, (PREGISTRY_SUBKEY_RECORD)DymArrayItem(dymArray, i));
				}
			}

			DymArrayDestroy(dymArray);
		}
	} else status = STATUS_NO_MORE_ENTRIES;

	ExReleaseResourceLite(&Record->HiddenSubkeyLock);
	KeLeaveCriticalRegion();
	if (NT_SUCCESS(status)) {
		*Array = tmpArray;
		*Count = tmpCount;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Array=0x%p, *Count=%u", status, *Array, *Count);
	return status;
}

VOID KeyRecordHiddenSubkeysDereference(_Inout_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_SUBKEY_RECORD *Array, _In_ SIZE_T Count)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p; Array=0x%p; Count=%u", Record, Array, Count);

	if (Count > 0) {
		ULONG i = 0;

		for (i = 0; i < Count; ++i)
			KeyRecordHiddenSubkeyDereference(Record, Array[i]);

		HeapMemoryFree(Array);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/*****************/
/* PSEUDO VALUES */
/*****************/

NTSTATUS KeyRecordAddPseudoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName, _In_ ULONG ValueType, _In_opt_ ULONG ValueDataLength, _In_opt_ PVOID ValueData, _In_ ERegistryValueOpMode DeleteMode, _In_ ERegistryValueOpMode ChangeMode, _In_opt_ PUNICODE_STRING ProcessName)
{
	USHORT processNameLen = 0;
	PREGISTRY_VALUE_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueName=\"%wZ\"; ValueType=%u; ValueDataLength=%u; ValueData=0x%p; DeleteMode=%u; ChangeMode=%u; ProcessName=\"%wZ\"", Record, ValueName, ValueType, ValueDataLength, ValueData, DeleteMode, ChangeMode, ProcessName);

	processNameLen = (ProcessName != NULL) ? ProcessName->Length : 0;
	tmpRecord = (PREGISTRY_VALUE_RECORD)HeapMemoryAllocPaged(sizeof(REGISTRY_VALUE_RECORD) + ValueName->Length);
	if (tmpRecord != NULL) {
		InitializeListHead(&tmpRecord->Entry);
		tmpRecord->ReferenceCount = 1;
		tmpRecord->DeletePending = FALSE;
		tmpRecord->ValueName = *ValueName;
		tmpRecord->ValueName.Buffer = (PWCH)(tmpRecord + 1);
		memcpy(tmpRecord->ValueName.Buffer, ValueName->Buffer, tmpRecord->ValueName.Length);
		status = ExInitializeResourceLite(&tmpRecord->DataLock);
		if (NT_SUCCESS(status)) {
			tmpRecord->ValueType = ValueType;
			tmpRecord->DataLength = ValueDataLength;
			tmpRecord->DeleteMode = DeleteMode;
			tmpRecord->ChangeMode = ChangeMode;
			memset(&tmpRecord->ProcessName, 0, sizeof(UNICODE_STRING));
			if (processNameLen > 0) {
				tmpRecord->ProcessName = *ProcessName;
				tmpRecord->ProcessName.Buffer = (PWCH)HeapMemoryAllocPaged(processNameLen);
				if (tmpRecord->ProcessName.Buffer != NULL)
					memcpy(tmpRecord->ProcessName.Buffer, ProcessName->Buffer, tmpRecord->ProcessName.Length);
				else status = STATUS_INSUFFICIENT_RESOURCES;
			}

			if (tmpRecord->DataLength > 0) {
				tmpRecord->Data = HeapMemoryAllocPaged(tmpRecord->DataLength);
				if (tmpRecord != NULL)
					memcpy(tmpRecord->Data, ValueData, tmpRecord->DataLength);
				else status = STATUS_INSUFFICIENT_RESOURCES;
			}

			if (NT_SUCCESS(status)) {
				status = ExInitializeResourceLite(&tmpRecord->ModeLock);
				if (NT_SUCCESS(status)) {
					status = ExInitializeResourceLite(&tmpRecord->ProcessNameLock);
					if (NT_SUCCESS(status)) {
						KeEnterCriticalRegion();
						ExAcquireResourceExclusiveLite(&Record->PseudoValueLock, TRUE);
						status = stringHashTableInsertUnicodeString(Record->PseudoValues, &tmpRecord->ValueName, tmpRecord);
						if (NT_SUCCESS(status)) {
							InterlockedIncrement(&tmpRecord->ReferenceCount);
							InsertTailList(&Record->PseudoValueListHead, &tmpRecord->Entry);
							KeyRecordPseudoValueDereference(Record, tmpRecord);
						}

						ExReleaseResourceLite(&Record->PseudoValueLock);
						KeLeaveCriticalRegion();
						if (!NT_SUCCESS(status))
							ExDeleteResourceLite(&tmpRecord->ProcessNameLock);
					}
					
					if (!NT_SUCCESS(status))
						ExDeleteResourceLite(&tmpRecord->ModeLock);
				}

				if (!NT_SUCCESS(status) && tmpRecord->DataLength > 0)
					HeapMemoryFree(tmpRecord->Data);
			}

			if (!NT_SUCCESS(status))
				ExDeleteResourceLite(&tmpRecord->DataLock);
		}

		if (!NT_SUCCESS(status))
			HeapMemoryFree(tmpRecord);
	} else status = STATUS_UNSUCCESSFUL;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS KeyRecordChangePseudoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREGISTRY_VALUE_RECORD ValueRecord, _In_ ULONG ValueType, _In_opt_ PVOID Data, _In_opt_ ULONG DataLength, _In_ ERegistryValueOpMode ChangeMode, _In_ ERegistryValueOpMode DeleteMode, _In_opt_ PUNICODE_STRING ProcessName)
{
	PVOID newData = NULL;
	UNICODE_STRING uProcessName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueRecord=0x%p; ValueType=%u; Data=0x%p; DataLength=%u; ChangeMode=%u; DeleteMode=%u; ProcessName=\"%wZ\"", Record, ValueRecord, ValueType, Data, DataLength, ChangeMode, DeleteMode, ProcessName);

	status = STATUS_SUCCESS;
	if (DataLength > 0) {
		newData = HeapMemoryAllocPaged(DataLength);
		if (newData != NULL)
			memcpy(newData, Data, DataLength);
		else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (NT_SUCCESS(status)) {
		memset(&uProcessName, 0, sizeof(UNICODE_STRING));
		if (ProcessName != NULL) {
			uProcessName.Length = ProcessName->Length;
			uProcessName.MaximumLength = uProcessName.Length;
			uProcessName.Buffer = (PWCH)HeapMemoryAllocPaged(uProcessName.Length);
			if (uProcessName.Buffer != NULL)
				memcpy(uProcessName.Buffer, ProcessName->Buffer, uProcessName.Length);
			else status = STATUS_INSUFFICIENT_RESOURCES;
		}

		if (NT_SUCCESS(status)) {
			KeEnterCriticalRegion();
			ExAcquireResourceExclusiveLite(&ValueRecord->ProcessNameLock, TRUE);
			if (ValueRecord->ProcessName.Length > 0)
				HeapMemoryFree(ValueRecord->ProcessName.Buffer);

			memcpy(&ValueRecord->ProcessName, &uProcessName, sizeof(UNICODE_STRING));
			ExReleaseResourceLite(&ValueRecord->ProcessNameLock);

			ExAcquireResourceExclusiveLite(&ValueRecord->ModeLock, TRUE);
			ValueRecord->ChangeMode = ChangeMode;
			ValueRecord->DeleteMode = DeleteMode;
			ExReleaseResourceLite(&ValueRecord->ModeLock);
			
			ExAcquireResourceExclusiveLite(&ValueRecord->DataLock, TRUE);
			if (ValueRecord->Data != NULL)
				HeapMemoryFree(ValueRecord->Data);

			ValueRecord->Data = newData;
			ValueRecord->DataLength = DataLength;
			ValueRecord->ValueType = ValueType;
			ExReleaseResourceLite(&ValueRecord->DataLock);
			
			KeLeaveCriticalRegion();
		}

		if (!NT_SUCCESS(status)) {
			if (newData != NULL)
				HeapMemoryFree(newData);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS KeyRecordDeletePsuedoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName)
{
	PREGISTRY_VALUE_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueName=\"%wZ\"", Record, ValueName);

	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&Record->PseudoValueLock, TRUE);
	tmpRecord = (PREGISTRY_VALUE_RECORD)StringHashTableDeleteUnicodeString(Record->PseudoValues, ValueName);
	ExReleaseResourceLite(&Record->PseudoValueLock);
	KeLeaveCriticalRegion();
	if (tmpRecord != NULL) {
		tmpRecord->DeletePending = TRUE;
		KeyRecordPseudoValueDereference(Record, tmpRecord);
		status = STATUS_SUCCESS;
	} else status = STATUS_OBJECT_NAME_NOT_FOUND;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


NTSTATUS KeyRecordGetPseudoValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PUNICODE_STRING ValueName, _Out_ PREGISTRY_VALUE_RECORD *ValueRecord)
{
	PREGISTRY_VALUE_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueName=\"%wZ\"; SubkeyRecord=0x%p", Record, ValueName, ValueRecord);

	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&Record->PseudoValueLock, TRUE);
	tmpRecord = (PREGISTRY_VALUE_RECORD)StringHashTableGetUnicodeString(Record->PseudoValues, ValueName);
	if (tmpRecord != NULL) {
		ASSERT(!tmpRecord->DeletePending);
		InterlockedIncrement(&tmpRecord->ReferenceCount);
		*ValueRecord = tmpRecord;
		status = STATUS_SUCCESS;
	} else status = STATUS_OBJECT_NAME_NOT_FOUND;

	ExReleaseResourceLite(&Record->PseudoValueLock);
	KeLeaveCriticalRegion();

	DEBUG_EXIT_FUNCTION("0x%x, *ValueRecord=0x%p", status, *ValueRecord);
	return status;
}

ULONG KeyRecordGetPseudoValuesCount(_In_ PREGISTRY_KEY_RECORD Record)
{
	ULONG ret = 0;
	PREGISTRY_VALUE_RECORD tmpRecord = NULL;
	PPROCESSDB_PROCESS_RECORD pr = NULL;
	DEBUG_ENTER_FUNCTION("Record=0x%p", Record);

	if (NT_SUCCESS(ProcessDBRecordGet(PsGetCurrentProcessId(), &pr))) {
		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&Record->PseudoValueLock, TRUE);
		tmpRecord = CONTAINING_RECORD(Record->PseudoValueListHead.Flink, REGISTRY_VALUE_RECORD, Entry);
		while (&tmpRecord->Entry != &Record->PseudoValueListHead) {
			if (!tmpRecord->DeletePending) {
				UNICODE_STRING uProcessNamePart;
				BOOLEAN countIn = FALSE;

				ExAcquireResourceSharedLite(&tmpRecord->ProcessNameLock, TRUE);
				countIn = (tmpRecord->ProcessName.Length == 0);
				if (!countIn) {
					if (pr->ImageFileName.Length >= tmpRecord->ProcessName.Length) {
						uProcessNamePart.Length =tmpRecord->ProcessName.Length;
						uProcessNamePart.MaximumLength = uProcessNamePart.Length;
						uProcessNamePart.Buffer = pr->ImageFileName.Buffer + ((pr->ImageFileName.Length - uProcessNamePart.Length) / sizeof(WCHAR));
						countIn = RtlEqualUnicodeString(&uProcessNamePart, &tmpRecord->ProcessName, TRUE);
					}
				}

				ExReleaseResourceLite(&tmpRecord->ProcessNameLock);
				if (countIn)
					++ret;;
			}

			tmpRecord = CONTAINING_RECORD(tmpRecord->Entry.Flink, REGISTRY_VALUE_RECORD, Entry);
		}

		ExReleaseResourceLite(&Record->PseudoValueLock);
		KeLeaveCriticalRegion();

		ProcessDBRecordDereference(pr);
	}


	DEBUG_EXIT_FUNCTION("%u", ret);
	return ret;
}

NTSTATUS KeyRecordGetPseudoValueByIndex(_In_ PREGISTRY_KEY_RECORD Record, _In_ ULONG Index, _Out_ PREGISTRY_VALUE_RECORD *ValueRecord)
{
	PPROCESSDB_PROCESS_RECORD pr = NULL;
	PREGISTRY_VALUE_RECORD tmpRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Index=%u; SubkeyRecord=0x%p", Record, Index, ValueRecord);

	status = ProcessDBRecordGet(PsGetCurrentProcessId(), &pr);
	if (NT_SUCCESS(status)) {
		status = STATUS_NOT_FOUND;
		KeEnterCriticalRegion();
		ExAcquireResourceSharedLite(&Record->PseudoValueLock, TRUE);
		tmpRecord = CONTAINING_RECORD(Record->PseudoValueListHead.Flink, REGISTRY_VALUE_RECORD, Entry);
		while (&tmpRecord->Entry != &Record->PseudoValueListHead) {
			if (!tmpRecord->DeletePending) {
				UNICODE_STRING uProcessNamePart;
				BOOLEAN countIn = FALSE;

				ExAcquireResourceSharedLite(&tmpRecord->ProcessNameLock, TRUE);
				countIn = (tmpRecord->ProcessName.Length == 0);
				if (!countIn) {
					if (pr->ImageFileName.Length >= tmpRecord->ProcessName.Length) {
						uProcessNamePart.Length = tmpRecord->ProcessName.Length;
						uProcessNamePart.MaximumLength = uProcessNamePart.Length;
						uProcessNamePart.Buffer = pr->ImageFileName.Buffer + ((pr->ImageFileName.Length - uProcessNamePart.Length) / sizeof(WCHAR));
						countIn = RtlEqualUnicodeString(&uProcessNamePart, &tmpRecord->ProcessName, TRUE);
					}
				}

				ExReleaseResourceLite(&tmpRecord->ProcessNameLock);
				if (countIn) {
					if (Index == 0) {
						InterlockedIncrement(&tmpRecord->ReferenceCount);
						*ValueRecord = tmpRecord;
						status = STATUS_SUCCESS;
						break;
					}

					--Index;
				}
			}

			tmpRecord = CONTAINING_RECORD(tmpRecord->Entry.Flink, REGISTRY_VALUE_RECORD, Entry);
		}

		ExReleaseResourceLite(&Record->PseudoValueLock);
		KeLeaveCriticalRegion();
		ProcessDBRecordDereference(pr);
	}

	DEBUG_EXIT_FUNCTION("0x%x, *ValueRecord=0x%p", status, *ValueRecord);
	return status;
}


VOID KeyRecordPseudoValueDereference(_In_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_VALUE_RECORD ValueRecord)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p; ValueRecord=0x%p", Record, ValueRecord);

	if (InterlockedDecrement(&ValueRecord->ReferenceCount) == 0) {
		KeEnterCriticalRegion();
		ExAcquireResourceExclusiveLite(&Record->PseudoValueLock, TRUE);
		RemoveEntryList(&ValueRecord->Entry);
		ExReleaseResourceLite(&Record->PseudoValueLock);
		KeLeaveCriticalRegion();
		ExDeleteResourceLite(&ValueRecord->ProcessNameLock);
		ExDeleteResourceLite(&ValueRecord->ModeLock);
		ExDeleteResourceLite(&ValueRecord->DataLock);
		if (ValueRecord->DataLength > 0)
			HeapMemoryFree(ValueRecord->Data);

		if (ValueRecord->ProcessName.Length > 0)
			HeapMemoryFree(ValueRecord->ProcessName.Buffer);

		HeapMemoryFree(ValueRecord);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS KeyRecordPseudoValuesEnumerate(_In_ PREGISTRY_KEY_RECORD Record, _Out_ PREGISTRY_VALUE_RECORD **Array, _Out_ PSIZE_T Count)
{
	SIZE_T tmpCount = 0;
	PREGISTRY_VALUE_RECORD *tmpArray = NULL;
	ULONG recordCount = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Array=0x%p; Count=0x%p", Record, Array, Count);

	status = STATUS_SUCCESS;
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&Record->PseudoValueLock, TRUE);
	recordCount = StringHashTableGetItemCount(Record->PseudoValues);
	if (recordCount > 0) {
		PUTILS_DYM_ARRAY dymArray = NULL;

		status = DymArrayCreate(PagedPool, &dymArray);
		if (NT_SUCCESS(status)) {
			status = DymArrayReserve(dymArray, recordCount);
			if (NT_SUCCESS(status)) {
				PREGISTRY_VALUE_RECORD valueRecord = NULL;

				valueRecord = CONTAINING_RECORD(Record->PseudoValueListHead.Flink, REGISTRY_VALUE_RECORD, Entry);
				while (&valueRecord->Entry != &Record->PseudoValueListHead) {
					if (!valueRecord->DeletePending) {
						InterlockedIncrement(&valueRecord->ReferenceCount);
						DymArrayPushBackNoAlloc(dymArray, valueRecord);
					}

					valueRecord = CONTAINING_RECORD(valueRecord->Entry.Flink, REGISTRY_VALUE_RECORD, Entry);
				}

				tmpCount = DymArrayLength(dymArray);
				status = DymArrayToStaticArrayAlloc(dymArray, PagedPool, (PVOID **)&tmpArray);
				if (!NT_SUCCESS(status)) {
					ULONG i = 0;

					for (i = 0; i < tmpCount; ++i)
						KeyRecordPseudoValueDereference(Record, (PREGISTRY_VALUE_RECORD)DymArrayItem(dymArray, i));
				}
			}

			DymArrayDestroy(dymArray);
		}
	} else status = STATUS_NO_MORE_ENTRIES;

	ExReleaseResourceLite(&Record->PseudoValueLock);
	KeLeaveCriticalRegion();
	if (NT_SUCCESS(status)) {
		*Array = tmpArray;
		*Count = tmpCount;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *Array=0x%p, *Count=%u", status, *Array, *Count);
	return status;
}

VOID KeyRecordPseudoValuesDereference(_Inout_ PREGISTRY_KEY_RECORD Record, _Inout_ PREGISTRY_VALUE_RECORD *Array, _In_ SIZE_T Count)
{
	DEBUG_ENTER_FUNCTION("Record=0x%p; Array=0x%p; Count=%u", Record, Array, Count);

	if (Count > 0) {
		ULONG i = 0;

		for (i = 0; i < Count; ++i)
			KeyRecordPseudoValueDereference(Record, Array[i]);

		HeapMemoryFree(Array);
	}

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                     INITIALIZATION AND FINALIZATION                  */
/************************************************************************/

NTSTATUS KeyRecordModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"", DriverObject, RegistryPath);

	InitializeListHead(&_keyRecordListHead);
	status = ExInitializeResourceLite(&_keyRecordLock);
	if (NT_SUCCESS(status)) {
		status = StringHashTableCreate(httNoSynchronization, 37, &_keyRecordTable);
		if (!NT_SUCCESS(status))
			ExDeleteResourceLite(&_keyRecordLock);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

VOID KeyRecordModuleFInit(_In_ PDRIVER_OBJECT DriverObject)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	HashTableDestroy(_keyRecordTable);
	ExDeleteResourceLite(&_keyRecordLock);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
