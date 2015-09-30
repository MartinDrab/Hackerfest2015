
#include <basetsd.h >
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "utils-dym-array.h"
#include "key-record.h"
#include "process-db.h"
#include "registry-callback.h"

/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static LARGE_INTEGER _registryCallbackCookie;
static UNICODE_STRING _altitude;

/************************************************************************/
/*                     HELPER FUNCTIONS                                 */
/************************************************************************/


static NTSTATUS _GetObjectName(_In_ PVOID Object, _Out_ POBJECT_NAME_INFORMATION *ObjectName)
{
	ULONG objectNameLen = 0;
	POBJECT_NAME_INFORMATION tmpObjectName = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Object=0x%p; ObjectName=0x%p", Object, ObjectName);

	*ObjectName = NULL;
	status = ObQueryNameString(Object, NULL, 0, &objectNameLen);
	if (status == STATUS_INFO_LENGTH_MISMATCH) {
		objectNameLen += sizeof(OBJECT_NAME_INFORMATION);
		tmpObjectName = (POBJECT_NAME_INFORMATION)HeapMemoryAllocPaged(objectNameLen);
		if (tmpObjectName != NULL) {
			status = ObQueryNameString(Object, tmpObjectName, objectNameLen, &objectNameLen);
			if (NT_SUCCESS(status))
				*ObjectName = tmpObjectName;

			if (!NT_SUCCESS(status))
				HeapMemoryFree(tmpObjectName);
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	} else if (NT_SUCCESS(status)) {
		objectNameLen = sizeof(OBJECT_NAME_INFORMATION);
		tmpObjectName = (POBJECT_NAME_INFORMATION)HeapMemoryAllocPaged(objectNameLen);
		if (tmpObjectName != NULL) {
			memset(tmpObjectName, 0, objectNameLen);
			*ObjectName = tmpObjectName;
			status = STATUS_SUCCESS;
		} else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	DEBUG_EXIT_FUNCTION("0x%x, *ObjectName=0x%p", status, *ObjectName);
	return status;
}


static VOID _FreeobjectName(_Inout_ POBJECT_NAME_INFORMATION ObjectName)
{
	DEBUG_ENTER_FUNCTION("ObjectName=0x%p", ObjectName);

	HeapMemoryFree(ObjectName);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}


static PVOID _GetKeyObject(_In_ REG_NOTIFY_CLASS OpType, _In_ PVOID OpInfo)
{
	PVOID ret = NULL;
	DEBUG_ENTER_FUNCTION("OpType=%u; OpInfo=0x%p", OpType, OpInfo);

	switch (OpType) {
		case RegNtPreQueryKey:
			ret = ((PREG_QUERY_KEY_INFORMATION)OpInfo)->Object;
			break;
		case RegNtPreEnumerateKey:
			ret = ((PREG_ENUMERATE_KEY_INFORMATION)OpInfo)->Object;
			break;

		case RegNtPreEnumerateValueKey:
			ret = ((PREG_ENUMERATE_VALUE_KEY_INFORMATION)OpInfo)->Object;
			break;
		case RegNtPreQueryValueKey:
			ret = ((PREG_QUERY_VALUE_KEY_INFORMATION)OpInfo)->Object;
			break;
		case RegNtPreDeleteValueKey:
			ret = ((PREG_DELETE_VALUE_KEY_INFORMATION)OpInfo)->Object;
			break;
		case RegNtPreSetValueKey:
			ret = ((PREG_SET_VALUE_KEY_INFORMATION)OpInfo)->Object;
			break;
		default:
			ASSERT(FALSE);
			break;
	}

	DEBUG_EXIT_FUNCTION("0x%p", ret);
	return ret;
}


static NTSTATUS _OnQueryKey(PREGISTRY_KEY_RECORD KeyRecord, PREG_QUERY_KEY_INFORMATION Info)
{
	ULONG pseudoValueCount = 0;
	ULONG hiddenKeyCount = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("KeyRecord=0x%p; Info=0x%p", KeyRecord, Info);

	status = STATUS_SUCCESS;
	if (Info->Length > 0 && Info->KeyInformation != NULL) {
		switch (Info->KeyInformationClass) {
			case KeyCachedInformation:
			case KeyFullInformation: {
				ULONG retLength = 0;
				HANDLE keyHandle = NULL;
				PVOID keyInformation = NULL;
				KPROCESSOR_MODE accessMode = ExGetPreviousMode();

				hiddenKeyCount = HashTableGetItemCount(KeyRecord->HiddenSubkeys);
				pseudoValueCount = KeyRecordGetPseudoValuesCount(KeyRecord);
				if (hiddenKeyCount + pseudoValueCount > 0) {
					status = ObOpenObjectByPointer(Info->Object, OBJ_KERNEL_HANDLE, NULL, KEY_QUERY_VALUE, *CmKeyObjectType, KernelMode, &keyHandle);
					if (NT_SUCCESS(status)) {
						keyInformation = HeapMemoryAllocPaged(Info->Length);
						if (keyInformation != NULL) {
							status = ZwQueryKey(keyHandle, Info->KeyInformationClass, keyInformation, Info->Length, &retLength);
							if (NT_SUCCESS(status)) {
								PKEY_CACHED_INFORMATION ci = (PKEY_CACHED_INFORMATION)keyInformation;
								PKEY_FULL_INFORMATION fi = (PKEY_FULL_INFORMATION)keyInformation;

								switch (Info->KeyInformationClass) {
									case KeyCachedInformation:
										ci->SubKeys -= hiddenKeyCount;
										ci->Values += pseudoValueCount;
										break;
									case KeyFullInformation:
										fi->SubKeys -= hiddenKeyCount;
										fi->Values += pseudoValueCount;
										break;
									default:
										ASSERT(FALSE);
										break;
								}

								if (accessMode == UserMode) {
									__try {
										memcpy(Info->KeyInformation, keyInformation, retLength);
										if (Info->ResultLength != NULL)
											*(Info->ResultLength) = retLength;
									} __except (EXCEPTION_EXECUTE_HANDLER) {
										status = GetExceptionCode();
									}
								} else {
									memcpy(Info->KeyInformation, keyInformation, retLength);
									if (Info->ResultLength != NULL)
										*(Info->ResultLength) = retLength;
								}

								if (NT_SUCCESS(status))
									status = STATUS_CALLBACK_BYPASS;
							}

							HeapMemoryFree(keyInformation);
						} else status = STATUS_INSUFFICIENT_RESOURCES;
					
						ZwClose(keyHandle);
					}
				}
			} break;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _OnEnumerateKey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREG_ENUMERATE_KEY_INFORMATION Info)
{
	ULONG index = 0;
	KPROCESSOR_MODE accessMode = ExGetPreviousMode();
	HANDLE keyHandle = NULL;
	ULONG kbiLen = 0;
	ULONG i = 0;
	PKEY_BASIC_INFORMATION kbi = NULL;
	PUTILS_DYM_ARRAY nameInfoArray = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p", Record, Info);

	status = DymArrayCreate(PagedPool, &nameInfoArray);
	if (NT_SUCCESS(status)) {
		status = ObOpenObjectByPointer(Info->Object, OBJ_KERNEL_HANDLE, NULL, KEY_ENUMERATE_SUB_KEYS, *CmKeyObjectType, KernelMode, &keyHandle);
		if (NT_SUCCESS(status)) {
			DEBUG_PRINT_LOCATION("Handle created = 0x%p", keyHandle);
			do {
				status = ZwEnumerateKey(keyHandle, index, KeyBasicInformation, NULL, 0, &kbiLen);
				if (status == STATUS_BUFFER_TOO_SMALL) {
					kbi = (PKEY_BASIC_INFORMATION)HeapMemoryAllocPaged(kbiLen);
					if (kbi != NULL) {
						status = ZwEnumerateKey(keyHandle, index, KeyBasicInformation, kbi, kbiLen, &kbiLen);
						if (NT_SUCCESS(status)) {
							status = DymArrayPushBack(nameInfoArray, kbi);
							++index;
						}

						if (!NT_SUCCESS(status))
							HeapMemoryFree(kbi);
					} else status = STATUS_INSUFFICIENT_RESOURCES;
				}
			} while (NT_SUCCESS(status));

			if (status == STATUS_NO_MORE_ENTRIES)
				status = STATUS_SUCCESS;

			if (NT_SUCCESS(status)) {
				ULONG subkeyCount = (ULONG)DymArrayLength(nameInfoArray);

				if (Info->Index < subkeyCount) {
					ULONG numberOfHidden = 0;
					ULONG lastHiddenIndex = 0xffffffff;

					for (i = 0; i < subkeyCount; ++i) {
						UNICODE_STRING uSubkeyName;
						PREGISTRY_SUBKEY_RECORD subkeyRecord = NULL;

						if (i > Info->Index && lastHiddenIndex != i - 1)
							break;

						kbi = (PKEY_BASIC_INFORMATION)DymArrayItem(nameInfoArray, i);
						uSubkeyName.Length = (USHORT)kbi->NameLength;
						uSubkeyName.MaximumLength = uSubkeyName.Length;
						uSubkeyName.Buffer = kbi->Name;
						status = KeyRecordGetHiddenSubkey(Record, &uSubkeyName, &subkeyRecord);
						if (NT_SUCCESS(status)) {
							++numberOfHidden;
							lastHiddenIndex = i;
							KeyRecordHiddenSubkeyDereference(Record, subkeyRecord);
						}
					}

					status = STATUS_SUCCESS;
					index = Info->Index + numberOfHidden;
					if (index < subkeyCount) {
						kbiLen = Info->Length;
						kbi = (PKEY_BASIC_INFORMATION)HeapMemoryAllocPaged(kbiLen);
						if (kbi != NULL) {
							ULONG kniRetLength = 0;
							ULONG bytesToCopy = 0;

							status = ZwEnumerateKey(keyHandle, index, Info->KeyInformationClass, kbi, kbiLen, &kniRetLength);
							if (NT_SUCCESS(status))
								bytesToCopy = kniRetLength;
							else if (status == STATUS_BUFFER_OVERFLOW)
								bytesToCopy = kbiLen;

							if (accessMode == UserMode) {
								__try {
									memcpy(Info->KeyInformation, kbi, bytesToCopy);
									*(Info->ResultLength) = kniRetLength;
								} __except (EXCEPTION_EXECUTE_HANDLER) {
									status = GetExceptionCode();
								}
							} else {
								memcpy(Info->KeyInformation, kbi, bytesToCopy);
								*(Info->ResultLength) = kniRetLength;
							}

							if (NT_SUCCESS(status))
								status = STATUS_CALLBACK_BYPASS;

							HeapMemoryFree(kbi);
						} else status = STATUS_INSUFFICIENT_RESOURCES;
					} else status = STATUS_NO_MORE_ENTRIES;
				} else status = STATUS_NO_MORE_ENTRIES;
			}

			for (i = 0; i < DymArrayLength(nameInfoArray); ++i)
				HeapMemoryFree(DymArrayItem(nameInfoArray, i));

			ZwClose(keyHandle);
		}

		DymArrayDestroy(nameInfoArray);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _QueryPseudoValue(_In_ PREGISTRY_VALUE_RECORD ValueRecord, _In_ KPROCESSOR_MODE AccessMode, _In_ KEY_VALUE_INFORMATION_CLASS InformationClass, _In_opt_ PVOID Buffer, _In_opt_ ULONG Length, _Out_opt_ PULONG ReturnLength)
{
	ULONG bytesToCopy = 0;
	ULONG retLength = 0;
	PVOID srcBuffer = NULL;
	ULONG valueDataLength = 0;
	ULONG valueNameLen = ValueRecord->ValueName.Length;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("ValueRecord=0x%p; AccessMode=%u; InformationClass=%u; Buffer=0x%p; Length=%u; ReturnLength=0x%p", ValueRecord, AccessMode, InformationClass, Buffer, Length, ReturnLength);

	status = STATUS_SUCCESS;
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&ValueRecord->DataLock, TRUE);
	valueDataLength = ValueRecord->DataLength;
	switch (InformationClass) {
		case KeyValueBasicInformation: {
			PKEY_VALUE_BASIC_INFORMATION vbi = NULL;

			vbi = (PKEY_VALUE_BASIC_INFORMATION)HeapMemoryAllocPaged(sizeof(KEY_VALUE_BASIC_INFORMATION) + valueNameLen);
			if (vbi != NULL) {
				vbi->NameLength = valueNameLen;
				memcpy(vbi->Name, ValueRecord->ValueName.Buffer, vbi->NameLength);
				vbi->TitleIndex = 0;
				vbi->Type = ValueRecord->ValueType;
				srcBuffer = vbi;
				retLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + vbi->NameLength;
			} else status = STATUS_INSUFFICIENT_RESOURCES;
		} break;
		case KeyValuePartialInformation:
		case KeyValuePartialInformationAlign64: {
			PKEY_VALUE_PARTIAL_INFORMATION vpi = NULL;

			vpi = (PKEY_VALUE_PARTIAL_INFORMATION)HeapMemoryAllocPaged(sizeof(KEY_VALUE_PARTIAL_INFORMATION) + valueDataLength);
			if (vpi != NULL) {
				vpi->DataLength = ValueRecord->DataLength;
				memcpy(vpi->Data, ValueRecord->Data, vpi->DataLength);
				vpi->TitleIndex = 0;
				vpi->Type = ValueRecord->ValueType;
				srcBuffer = vpi;
				retLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + vpi->DataLength;
			} else status = STATUS_INSUFFICIENT_RESOURCES;
		} break;
		case KeyValueFullInformation:
		case KeyValueFullInformationAlign64: {
			PKEY_VALUE_FULL_INFORMATION vfi = NULL;

			vfi = (PKEY_VALUE_FULL_INFORMATION)HeapMemoryAllocPaged(sizeof(KEY_VALUE_FULL_INFORMATION) + valueNameLen + valueDataLength);
			if (vfi != NULL) {
				vfi->NameLength = valueNameLen;
				memcpy(vfi->Name, ValueRecord->ValueName.Buffer, vfi->NameLength);
				vfi->DataLength = valueDataLength;
				vfi->DataOffset = sizeof(KEY_VALUE_FULL_INFORMATION) + vfi->NameLength;
				memcpy((PUCHAR)vfi + vfi->DataOffset, ValueRecord->Data, vfi->DataLength);
				vfi->TitleIndex = 0;
				vfi->Type = ValueRecord->ValueType;
				srcBuffer = vfi;
				retLength = sizeof(KEY_VALUE_FULL_INFORMATION) + vfi->DataLength + vfi->NameLength;
			} else status = STATUS_INSUFFICIENT_RESOURCES;
		} break;
		default:
			DEBUG_ERROR("Unknown information class %u", InformationClass);
			__debugbreak();
			break;
	}

	ExReleaseResourceLite(&ValueRecord->DataLock);
	KeLeaveCriticalRegion();
	if (NT_SUCCESS(status)) {
		if (InformationClass == KeyValueFullInformationAlign64 || InformationClass == KeyValuePartialInformationAlign64) {
			ULONG_PTR diff = (ULONG_PTR)ALIGN_UP_POINTER_BY(Buffer, sizeof(ULONG64)) - (ULONG_PTR)Buffer;
		
			Buffer = (PUCHAR)Buffer + diff;
			if (Length > diff)
				Length -= (ULONG)diff;
			else Length = 0;
		}

		bytesToCopy = (Length < retLength) ? Length : retLength;
		if (bytesToCopy < retLength)
			status = (Length == 0) ? STATUS_BUFFER_TOO_SMALL : STATUS_BUFFER_OVERFLOW;

		if (AccessMode == UserMode) {
			__try {
				if (Length > 0)
					memcpy(Buffer, srcBuffer, bytesToCopy);

				if (ReturnLength != NULL)
					*ReturnLength = retLength;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				status = GetExceptionCode();
			}
		} else {
			if (Length > 0)
				memcpy(Buffer, srcBuffer, bytesToCopy);

			if (ReturnLength != NULL)
				*ReturnLength = retLength;
		}

		if (srcBuffer != NULL)
			HeapMemoryFree(srcBuffer);
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _OnEnumerateValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREG_ENUMERATE_VALUE_KEY_INFORMATION Info)
{
	ULONG pseudoValueCount = 0;
	KPROCESSOR_MODE accessMode = ExGetPreviousMode();
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p", Record, Info);

	status = STATUS_SUCCESS;
	pseudoValueCount = KeyRecordGetPseudoValuesCount(Record);
	if (Info->Index < pseudoValueCount) {
		PREGISTRY_VALUE_RECORD valueRecord = NULL;

		status = KeyRecordGetPseudoValueByIndex(Record, Info->Index, &valueRecord);
		if (NT_SUCCESS(status)) {
			status = _QueryPseudoValue(valueRecord, accessMode, Info->KeyValueInformationClass, Info->KeyValueInformation, Info->Length, Info->ResultLength);
			if (NT_SUCCESS(status))
				status = STATUS_CALLBACK_BYPASS;
			
			KeyRecordPseudoValueDereference(Record, valueRecord);
		} else status = STATUS_SUCCESS;
	} else {
		HANDLE keyHandle = NULL;

		status = ObOpenObjectByPointer(Info->Object, OBJ_KERNEL_HANDLE, NULL, KEY_QUERY_VALUE, *CmKeyObjectType, KernelMode, &keyHandle);
		if (NT_SUCCESS(status)) {
			ULONG retLength = 0;
			PVOID infoBuffer = NULL;

			if (Info->KeyValueInformation != NULL)
				infoBuffer = HeapMemoryAllocPaged(Info->Length);
			
			if (infoBuffer != NULL || Info->KeyValueInformation == NULL) {
				ULONG bytesToCopy = 0;
				
				status = ZwEnumerateValueKey(keyHandle, Info->Index - pseudoValueCount, Info->KeyValueInformationClass, infoBuffer, Info->Length, &retLength);
				bytesToCopy = (Info->Length < retLength) ? Info->Length : retLength;
				if (accessMode == UserMode) {
					__try {
						if (Info->KeyValueInformation != NULL)
							memcpy(Info->KeyValueInformation, infoBuffer, bytesToCopy);

						if (Info->ResultLength != NULL)
							*(Info->ResultLength) = retLength;
					} __except (EXCEPTION_EXECUTE_HANDLER) {
						status = GetExceptionCode();
					}
				} else {
					if (Info->KeyValueInformation != NULL)
						memcpy(Info->KeyValueInformation, infoBuffer, bytesToCopy);

					if (Info->ResultLength != NULL)
						*(Info->ResultLength) = retLength;
				}

				if (NT_SUCCESS(status))
					status = STATUS_CALLBACK_BYPASS;

				if (infoBuffer != NULL)
					HeapMemoryFree(infoBuffer);
			} else status = STATUS_INSUFFICIENT_RESOURCES;

			ZwClose(keyHandle);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _CaptureUnicodeString(_In_ KPROCESSOR_MODE AccessMode, _In_ PUNICODE_STRING U, _Out_ PUNICODE_STRING *Captured)
{
	USHORT len = 0;
	PUNICODE_STRING tmpCaptured = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("AccessMode=%u; U=0x%p; Captured=0x%p", AccessMode, U, Captured);

	status = STATUS_SUCCESS;
	if (U != NULL) {
		if (AccessMode == UserMode) {
			__try {
				len = U->Length;
				tmpCaptured = (PUNICODE_STRING)HeapMemoryAllocPaged(sizeof(UNICODE_STRING) + len);
				if (tmpCaptured != NULL) {
					tmpCaptured->Length = len;
					tmpCaptured->MaximumLength = tmpCaptured->Length;
					tmpCaptured->Buffer = (PWCH)(tmpCaptured + 1);
					memcpy(tmpCaptured->Buffer, U->Buffer, tmpCaptured->Length);
				} else status = STATUS_INSUFFICIENT_RESOURCES;
			} __except (EXCEPTION_EXECUTE_HANDLER) {
				status = GetExceptionCode();
				if (tmpCaptured != NULL)
					HeapMemoryFree(tmpCaptured);
			}
		} else {
			len = U->Length;
			tmpCaptured = (PUNICODE_STRING)HeapMemoryAllocPaged(sizeof(UNICODE_STRING) + len);
			if (tmpCaptured != NULL) {
				tmpCaptured->Length = len;
				tmpCaptured->MaximumLength = tmpCaptured->Length;
				tmpCaptured->Buffer = (PWCH)(tmpCaptured + 1);
				memcpy(tmpCaptured->Buffer, U->Buffer, tmpCaptured->Length);
			} else status = STATUS_INSUFFICIENT_RESOURCES;
		}
	} else {
		tmpCaptured = (PUNICODE_STRING)HeapMemoryAllocPaged(sizeof(UNICODE_STRING));
		if (tmpCaptured != NULL)
			memset(tmpCaptured, 0, sizeof(tmpCaptured));
		else status = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (NT_SUCCESS(status))
		*Captured = tmpCaptured;

	DEBUG_EXIT_FUNCTION("0x%x, *Captured=\"%wZ\"", status, *Captured);
	return status;
}

static NTSTATUS _OnQueryValue(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREG_QUERY_VALUE_KEY_INFORMATION Info)
{
	PUNICODE_STRING uValueName;
	PREGISTRY_VALUE_RECORD valueRecord = NULL;
	KPROCESSOR_MODE accessMode = ExGetPreviousMode();
	ULONG pseudoValueCOunt = 0;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p", Record, Info);

	status = STATUS_SUCCESS;
	pseudoValueCOunt = HashTableGetItemCount(Record->PseudoValues);
	if (pseudoValueCOunt > 0) {
		status = _CaptureUnicodeString(accessMode, Info->ValueName, &uValueName);
		if (NT_SUCCESS(status)) {
			status = KeyRecordGetPseudoValue(Record, uValueName, &valueRecord);
			if (NT_SUCCESS(status)) {
				status = _QueryPseudoValue(valueRecord, accessMode, Info->KeyValueInformationClass, Info->KeyValueInformation, Info->Length, Info->ResultLength);
				if (NT_SUCCESS(status))
					status = STATUS_CALLBACK_BYPASS;

				KeyRecordPseudoValueDereference(Record, valueRecord);
			} else status = STATUS_SUCCESS;

			HeapMemoryFree(uValueName);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _OnSetValueKey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREG_SET_VALUE_KEY_INFORMATION Info)
{
	PUNICODE_STRING uValueName = NULL;
	PREGISTRY_VALUE_RECORD valueRecord = NULL;
	ULONG pseudoValueCOunt = 0;
	KPROCESSOR_MODE accessMode = ExGetPreviousMode();
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p", Record, Info);

	status = STATUS_SUCCESS;
	pseudoValueCOunt = HashTableGetItemCount(Record->PseudoValues);
	if (pseudoValueCOunt > 0) {
		status = _CaptureUnicodeString(accessMode, Info->ValueName, &uValueName);
		if (NT_SUCCESS(status)) {
			status = KeyRecordGetPseudoValue(Record, uValueName, &valueRecord);
			if (NT_SUCCESS(status)) {
				switch (valueRecord->ChangeMode) {
					case rvdmAllow: {
						PVOID newData = NULL;

						if (Info->DataSize > 0)
							newData = HeapMemoryAllocPaged(Info->DataSize);

						if (newData != NULL || Info->DataSize == 0) {
							if (Info->DataSize > 0) {
								if (accessMode == UserMode) {
									__try {
										memcpy(newData, Info->Data, Info->DataSize);
									} __except (EXCEPTION_EXECUTE_HANDLER) {
										status = GetExceptionCode();
									}
								} else memcpy(newData, Info->Data, Info->DataSize);
							}

							if (NT_SUCCESS(status)) {
								KeEnterCriticalRegion();
								ExAcquireResourceExclusiveLite(&valueRecord->DataLock, TRUE);
								if (valueRecord->Data != NULL)
									HeapMemoryFree(valueRecord->Data);

								valueRecord->DataLength = Info->DataSize;
								valueRecord->Data = newData;
								valueRecord->ValueType = Info->Type;
								ExReleaseResourceLite(&valueRecord->DataLock);
								KeLeaveCriticalRegion();
							}

							if (!NT_SUCCESS(status) && Info->DataSize > 0)
								HeapMemoryFree(newData);
						} else status = STATUS_INSUFFICIENT_RESOURCES;
					} break;
					case rvdmDeny:
						status = STATUS_ACCESS_DENIED;
						break;
					case rvdmPretend:
						break;
				}

				KeyRecordPseudoValueDereference(Record, valueRecord);
				if (NT_SUCCESS(status))
					status = STATUS_CALLBACK_BYPASS;
			} else status = STATUS_SUCCESS;

			HeapMemoryFree(uValueName);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _OnDeleteValueKey(_In_ PREGISTRY_KEY_RECORD Record, _In_ PREG_DELETE_VALUE_KEY_INFORMATION Info)
{
	PUNICODE_STRING uValueName = NULL;
	PREGISTRY_VALUE_RECORD valueRecord = NULL;
	ULONG pseudoValueCOunt = 0;
	KPROCESSOR_MODE accessMode = ExGetPreviousMode();
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("Record=0x%p; Info=0x%p", Record, Info);

	status = STATUS_SUCCESS;
	pseudoValueCOunt = HashTableGetItemCount(Record->PseudoValues);
	if (pseudoValueCOunt > 0) {
		status = _CaptureUnicodeString(accessMode, Info->ValueName, &uValueName);
		if (NT_SUCCESS(status)) {
			status = KeyRecordGetPseudoValue(Record, uValueName, &valueRecord);
			if (NT_SUCCESS(status)) {
				switch (valueRecord->DeleteMode) {
					case rvdmDeny:
						status = STATUS_ACCESS_DENIED;
						break;
					case rvdmAllow:
						status = KeyRecordDeletePsuedoValue(Record, uValueName);;
						break;
					case rvdmPretend:
						break;
					default:
						ASSERT(FALSE);
						break;
				}

				KeyRecordPseudoValueDereference(Record, valueRecord);
				if (NT_SUCCESS(status))
					status = STATUS_CALLBACK_BYPASS;
			} else status = STATUS_SUCCESS;

			HeapMemoryFree(uValueName);
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static NTSTATUS _RegistryCallback(_In_ PVOID CallbackContext, _In_opt_ PVOID Argument1, _In_opt_ PVOID Argument2)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	REG_NOTIFY_CLASS opType = (REG_NOTIFY_CLASS)(ULONG_PTR)(Argument1);
	REG_NOTIFY_CLASS interestingTypes[] = {
		RegNtPreQueryKey,
		RegNtPreEnumerateKey,

		RegNtPreEnumerateValueKey,
		RegNtPreQueryValueKey,
		RegNtPreDeleteValueKey,
		RegNtPreSetValueKey,
	};
	ULONG i = 0;
	BOOLEAN isInteresting = FALSE;
	DEBUG_ENTER_FUNCTION("CallbackContext=0x%p; Argument1=0x%p; Argument2=0x%p", CallbackContext, Argument1, Argument2);

	status = STATUS_SUCCESS;
	for (i = 0; i < sizeof(interestingTypes) / sizeof(REG_NOTIFY_CLASS); ++i) {
		isInteresting = (opType == interestingTypes[i]);
		if (isInteresting) {
			PVOID keyObject = NULL;
			POBJECT_NAME_INFORMATION objectName = NULL;

			keyObject = _GetKeyObject(opType, Argument2);
			ASSERT(keyObject != NULL);
			status = _GetObjectName(keyObject, &objectName);
			if (NT_SUCCESS(status)) {
				PREGISTRY_KEY_RECORD keyRecord = NULL;

				status = KeyRecordGet(&objectName->Name, &keyRecord);
				if (NT_SUCCESS(status)) {
					switch (opType) {
						case RegNtPreQueryKey:
							status = _OnQueryKey(keyRecord, (PREG_QUERY_KEY_INFORMATION)Argument2);
							break;
						case RegNtPreEnumerateKey:
							status = _OnEnumerateKey(keyRecord, (PREG_ENUMERATE_KEY_INFORMATION)Argument2);
							break;

						case RegNtPreEnumerateValueKey:
							status = _OnEnumerateValue(keyRecord, (PREG_ENUMERATE_VALUE_KEY_INFORMATION)Argument2);
							break;
						case RegNtPreQueryValueKey:
							status = _OnQueryValue(keyRecord, (PREG_QUERY_VALUE_KEY_INFORMATION)Argument2);
							break;
						case RegNtPreDeleteValueKey:
							status = _OnDeleteValueKey(keyRecord, (PREG_DELETE_VALUE_KEY_INFORMATION)Argument2);
							break;
						case RegNtPreSetValueKey:
							status = _OnSetValueKey(keyRecord, (PREG_SET_VALUE_KEY_INFORMATION)Argument2);
							break;
						default:
							ASSERT(FALSE);
							break;
					}

					KeyRecordDereference(keyRecord);
				} else status = STATUS_SUCCESS;

				_FreeobjectName(objectName);
			}

			break;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/


/************************************************************************/
/*                 INITIALIZATION AND FINALIZATION                      */
/************************************************************************/

NTSTATUS RegistryCallbackModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"", DriverObject, RegistryPath);

	RtlInitUnicodeString(&_altitude, L"40000");
	status = CmRegisterCallbackEx(_RegistryCallback, &_altitude, DriverObject, NULL, &_registryCallbackCookie, NULL);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


VOID RegistryCallbackModuleFinit(_In_ PDRIVER_OBJECT DriverObject)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	CmUnRegisterCallback(_registryCallbackCookie);

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}
