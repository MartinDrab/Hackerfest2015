
#include <ntifs.h>
#include "preprocessor.h"
#include "allocator.h"
#include "reghider-ioctl.h"
#include "key-record.h"
#include "um-services.h"

/************************************************************************/
/*                         PUBLIC FUNCTIONS                             */
/************************************************************************/

/***************/
/* HIDDEN KEYS */
/***************/

NTSTATUS UMHiddenKeyAdd(_In_ PIOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT InputBuffer, _In_ ULONG InputBufferLength)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("InputBuffer=0x%; InputBufferLength=%u", InputBuffer, InputBufferLength);

	if (InputBufferLength >= (ULONG)InputBuffer->KeyNameLength + FIELD_OFFSET(IOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT, KeyName)) {
		UNICODE_STRING uKeyName;
		PREGISTRY_KEY_RECORD keyRecord = NULL;
		USHORT parentNameLen = InputBuffer->KeyNameLength - sizeof(WCHAR);

		while (InputBuffer->KeyName[parentNameLen / sizeof(WCHAR)] != L'\\')
			parentNameLen -= sizeof(WCHAR);

		uKeyName.Length = parentNameLen;
		uKeyName.MaximumLength = uKeyName.Length;
		uKeyName.Buffer = InputBuffer->KeyName;
		status = KeyRecordGet(&uKeyName, &keyRecord);
		if (status == STATUS_OBJECT_NAME_NOT_FOUND)
			status = KeyRecordCreate(&uKeyName, &keyRecord);

		if (NT_SUCCESS(status)) {
			UNICODE_STRING uSubkeyName;

			uSubkeyName.Length = (InputBuffer->KeyNameLength - parentNameLen - sizeof(WCHAR));
			uSubkeyName.MaximumLength = uSubkeyName.Length;
			uSubkeyName.Buffer = InputBuffer->KeyName + ((parentNameLen / sizeof(WCHAR)) + 1);
			status = KeyRecordAddHiddenSubkey(keyRecord, &uSubkeyName, NULL);
			KeyRecordDereference(keyRecord);
		}
	} else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS UMHiddenKeyEnum(_Out_ PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT OutputBuffer, _In_ ULONG OutputBufferLength)
{
	SIZE_T subkeyRecordCount = 0;
	PREGISTRY_SUBKEY_RECORD *subkeyRecordArray = NULL;
	SIZE_T keyRecordCount = 0;
	PREGISTRY_KEY_RECORD *keyRecordArray = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT lastEntry = NULL;
	DEBUG_ENTER_FUNCTION("OutputBuffer=0x%p; OutputBufferLength=%u", OutputBuffer, OutputBufferLength);

	status = STATUS_SUCCESS;
	if (ExGetPreviousMode() == UserMode) {
		__try {
			ProbeForWrite(OutputBuffer, OutputBufferLength, 1);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
		}
	}

	if (NT_SUCCESS(status)) {
		status = KeyRecordsEnumerate(&keyRecordArray, &keyRecordCount);
		if (NT_SUCCESS(status)) {
			ULONG i = 0;
			PREGISTRY_KEY_RECORD keyRecord = NULL;

			for (i = 0; i < keyRecordCount; ++i) {
				keyRecord = keyRecordArray[i];
				status = KeyRecordHiddenSubkeysEnumerate(keyRecord, &subkeyRecordArray, &subkeyRecordCount);
				if (NT_SUCCESS(status)) {
					ULONG j = 0;
					PREGISTRY_SUBKEY_RECORD subkeyRecord = NULL;

					for (j = 0; j < subkeyRecordCount; ++j) {
						ULONG requiredLength = 0;

						subkeyRecord = subkeyRecordArray[j];
						requiredLength = sizeof(IOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT) + keyRecord->Name.Length + sizeof(WCHAR) + subkeyRecord->Name.Length;
						if (requiredLength <= OutputBufferLength) {
							OutputBufferLength -= requiredLength;
							__try {
								OutputBuffer->NextEntryOffset = requiredLength;
								OutputBuffer->KeyNameLength = keyRecord->Name.Length + sizeof(WCHAR) + subkeyRecord->Name.Length;
								memcpy(OutputBuffer->KeyName, keyRecord->Name.Buffer, keyRecord->Name.Length);
								OutputBuffer->KeyName[keyRecord->Name.Length / sizeof(WCHAR)] = L'\\';
								memcpy(&OutputBuffer->KeyName[(keyRecord->Name.Length / sizeof(WCHAR)) + 1], subkeyRecord->Name.Buffer, subkeyRecord->Name.Length);
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								status = GetExceptionCode();
							}

							lastEntry = OutputBuffer;
							OutputBuffer = (PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT)((PUCHAR)OutputBuffer + requiredLength);
						} else status = STATUS_BUFFER_TOO_SMALL;

						if (!NT_SUCCESS(status))
							break;
					}

					KeyRecordHiddenSubkeysDereference(keyRecord, subkeyRecordArray, subkeyRecordCount);
				}

				if (status == STATUS_NO_MORE_ENTRIES)
					status = STATUS_SUCCESS;

				if (!NT_SUCCESS(status))
					break;
			}

			KeyRecordsDereference(keyRecordArray, keyRecordCount);
			if (NT_SUCCESS(status)) {
				if (lastEntry != NULL) {
					__try {
						lastEntry->NextEntryOffset = 0;
					} __except (EXCEPTION_EXECUTE_HANDLER) {
						status = GetExceptionCode();
					}
				} else status = STATUS_NO_MORE_ENTRIES;
			}
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS UMHiddenKeyDelete(_In_ PIOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT InputBuffer, ULONG _In_ InputBufferLength)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("InputBuffer=0x%p; InputBufferLength=%u", InputBuffer, InputBufferLength);

	if (InputBufferLength >= (ULONG)InputBuffer->KeyNameLength + FIELD_OFFSET(IOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT, KeyName)) {
		UNICODE_STRING uKeyName;
		PREGISTRY_KEY_RECORD keyRecord = NULL;
		USHORT parentNameLen = InputBuffer->KeyNameLength - sizeof(WCHAR);

		while (InputBuffer->KeyName[parentNameLen / sizeof(WCHAR)] != L'\\')
			parentNameLen -= sizeof(WCHAR);

		uKeyName.Length = parentNameLen;
		uKeyName.MaximumLength = uKeyName.Length;
		uKeyName.Buffer = InputBuffer->KeyName;
		status = KeyRecordGet(&uKeyName, &keyRecord);
		if (NT_SUCCESS(status)) {
			UNICODE_STRING uSubkeyName;

			uSubkeyName.Length = (InputBuffer->KeyNameLength - parentNameLen - sizeof(WCHAR));
			uSubkeyName.MaximumLength = uSubkeyName.Length;
			uSubkeyName.Buffer = InputBuffer->KeyName + ((parentNameLen / sizeof(WCHAR)) + 1);
			status = KeyRecordDeleteHiddenSubkey(keyRecord, &uSubkeyName);
			KeyRecordDereference(keyRecord);
		}
	}
	else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

/*****************/
/* PSEUDO VALUES */
/*****************/

NTSTATUS UMPseudoValueAdd(_In_ PIOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT InputBuffer, _In_ ULONG InputBufferLength)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("InputBuffer=0x%p; InputBufferLength=%u", InputBuffer, InputBufferLength);

	if (InputBuffer->KeyNameOffset + InputBuffer->KeyNameLength <= InputBufferLength &&
		InputBuffer->NameOffset + InputBuffer->NameLength <= InputBufferLength &&
		InputBuffer->DataOffset + InputBuffer->DataLength <= InputBufferLength) {
		UNICODE_STRING uKeyName;
		PREGISTRY_KEY_RECORD keyRecord = NULL;

		uKeyName.Length = InputBuffer->KeyNameLength;
		uKeyName.MaximumLength = uKeyName.Length;
		uKeyName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->KeyNameOffset);
		status = KeyRecordGet(&uKeyName, &keyRecord);
		if (status == STATUS_OBJECT_NAME_NOT_FOUND)
			status = KeyRecordCreate(&uKeyName, &keyRecord);

		if (NT_SUCCESS(status)) {
			PVOID valueData = (PUCHAR)InputBuffer + InputBuffer->DataOffset;
			UNICODE_STRING uValueName;
			UNICODE_STRING uProcessName;

			uValueName.Length = InputBuffer->NameLength;
			uValueName.MaximumLength = uValueName.Length;
			uValueName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->NameOffset);
			uProcessName.Length = InputBuffer->ProcessNameLength;
			uProcessName.MaximumLength = uProcessName.Length;
			uProcessName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->ProcessNameOffset);
			status = KeyRecordAddPseudoValue(keyRecord, &uValueName, InputBuffer->ValueType, InputBuffer->DataLength, valueData, InputBuffer->DeleteMode, InputBuffer->ChangeMode, &uProcessName);
			KeyRecordDereference(keyRecord);
		}
	} else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS UMPseudoValueEnum(_Out_ PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT OutputBuffer, _In_ ULONG OutputBufferLength)
{
	SIZE_T valueRecordCount = 0;
	PREGISTRY_VALUE_RECORD *valueRecordArray = NULL;
	SIZE_T keyRecordCount = 0;
	PREGISTRY_KEY_RECORD *keyRecordArray = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT lastEntry = NULL;
	DEBUG_ENTER_FUNCTION("OutputBuffer=0x%p; OutputBufferLength=%u", OutputBuffer, OutputBufferLength);

	status = STATUS_SUCCESS;
	if (ExGetPreviousMode() == UserMode) {
		__try {
			ProbeForWrite(OutputBuffer, OutputBufferLength, 1);
		} __except (EXCEPTION_EXECUTE_HANDLER) {
			status = GetExceptionCode();
		}
	}

	if (NT_SUCCESS(status)) {
		status = KeyRecordsEnumerate(&keyRecordArray, &keyRecordCount);
		if (NT_SUCCESS(status)) {
			ULONG i = 0;
			PREGISTRY_KEY_RECORD keyRecord = NULL;

			for (i = 0; i < keyRecordCount; ++i) {
				keyRecord = keyRecordArray[i];
				status = KeyRecordPseudoValuesEnumerate(keyRecord, &valueRecordArray, &valueRecordCount);
				if (NT_SUCCESS(status)) {
					ULONG j = 0;
					PREGISTRY_VALUE_RECORD valueRecord = NULL;

					for (j = 0; j < valueRecordCount; ++j) {
						ULONG requiredLength = 0;

						valueRecord = valueRecordArray[j];
						requiredLength = sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT) + keyRecord->Name.Length + valueRecord->ValueName.Length + valueRecord->DataLength + valueRecord->ProcessName.Length;
						if (requiredLength <= OutputBufferLength) {
							BOOLEAN modeLockHeld = FALSE;
							BOOLEAN lockHeld = FALSE;
							BOOLEAN processLockHeld = FALSE;
							ULONG keyNameOffset = sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT);
							ULONG valueNameOffset = keyNameOffset + keyRecord->Name.Length;
							ULONG valueDataOffset = valueNameOffset + valueRecord->ValueName.Length;
							ULONG processNameOffset = valueDataOffset + valueRecord->DataLength;

							OutputBufferLength -= requiredLength;
							KeEnterCriticalRegion();
							__try {
								OutputBuffer->NextEntryOffset = requiredLength;
								
								ExAcquireResourceSharedLite(&valueRecord->ModeLock, TRUE);
								modeLockHeld = TRUE;
								OutputBuffer->ChangeMode = valueRecord->ChangeMode;
								OutputBuffer->DeleteMode = valueRecord->DeleteMode;
								ExReleaseResourceLite(&valueRecord->ModeLock);
								modeLockHeld = FALSE;

								ExAcquireResourceSharedLite(&valueRecord->DataLock, TRUE);
								lockHeld = TRUE;
								OutputBuffer->Valuetype = valueRecord->ValueType;
								OutputBuffer->DataOffset = valueDataOffset;
								OutputBuffer->DataLength = valueRecord->DataLength;
								memcpy((PUCHAR)OutputBuffer + valueDataOffset, valueRecord->Data, valueRecord->DataLength);
								ExReleaseResourceLite(&valueRecord->DataLock);
								lockHeld = FALSE;

								OutputBuffer->KeyNameOffset = keyNameOffset;
								OutputBuffer->KeyNameLength = keyRecord->Name.Length;
								memcpy((PUCHAR)OutputBuffer + keyNameOffset, keyRecord->Name.Buffer, keyRecord->Name.Length);

								OutputBuffer->NameOffset = valueNameOffset;
								OutputBuffer->NameLength = valueRecord->ValueName.Length;
								memcpy((PUCHAR)OutputBuffer + valueNameOffset, valueRecord->ValueName.Buffer, valueRecord->ValueName.Length);

								ExAcquireResourceSharedLite(&valueRecord->ProcessNameLock, TRUE);
								processLockHeld = TRUE;
								OutputBuffer->ProcessNameOffset = processNameOffset;
								OutputBuffer->ProcessNameLength = valueRecord->ProcessName.Length;
								memcpy((PUCHAR)OutputBuffer + processNameOffset, valueRecord->ProcessName.Buffer, valueRecord->ProcessName.Length);
								ExReleaseResourceLite(&valueRecord->ProcessNameLock);
								processLockHeld = FALSE;
							} __except (EXCEPTION_EXECUTE_HANDLER) {
								status = GetExceptionCode();
								if (processLockHeld)
									ExReleaseResourceLite(&valueRecord->ProcessNameLock);

								if (lockHeld)
									ExReleaseResourceLite(&valueRecord->DataLock);

								if (modeLockHeld)
									ExReleaseResourceLite(&valueRecord->ModeLock);
							}

							KeLeaveCriticalRegion();
							lastEntry = OutputBuffer;
							OutputBuffer = (PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT)((PUCHAR)OutputBuffer + requiredLength);
						} else status = STATUS_BUFFER_TOO_SMALL;

						if (!NT_SUCCESS(status))
							break;
					}

					KeyRecordPseudoValuesDereference(keyRecord, valueRecordArray, valueRecordCount);
				}

				if (status == STATUS_NO_MORE_ENTRIES)
					status = STATUS_SUCCESS;

				if (!NT_SUCCESS(status))
					break;
			}

			KeyRecordsDereference(keyRecordArray, keyRecordCount);
			if (NT_SUCCESS(status)) {
				if (lastEntry != NULL) {
					__try {
						lastEntry->NextEntryOffset = 0;
					} __except (EXCEPTION_EXECUTE_HANDLER) {
						status = GetExceptionCode();
					}
				} else status = STATUS_NO_MORE_ENTRIES;
			}
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS UMPseudoValueDelete(_In_ PIOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT InputBuffer, _In_ ULONG InputBufferLength)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("InputBuffer=0x%p; InputBufferLength=%u", InputBuffer, InputBufferLength);

	if (InputBufferLength >= sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT) &&
		InputBuffer->KeyNameOffset + InputBuffer->KeyNameLength <= InputBufferLength &&
		InputBuffer->ValueNameOffset + InputBuffer->ValueNameLength <= InputBufferLength) {
		UNICODE_STRING uKeyName;
		PREGISTRY_KEY_RECORD keyRecord = NULL;

		uKeyName.Length = InputBuffer->KeyNameLength;
		uKeyName.MaximumLength = uKeyName.Length;
		uKeyName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->KeyNameOffset);
		status = KeyRecordGet(&uKeyName, &keyRecord);
		if (NT_SUCCESS(status)) {
			UNICODE_STRING uValueName;

			uValueName.Length = InputBuffer->ValueNameLength;
			uValueName.MaximumLength = uValueName.Length;
			uValueName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->ValueNameOffset);
			status = KeyRecordDeletePsuedoValue(keyRecord, &uValueName);
			KeyRecordDereference(keyRecord);
		}
	} else status = STATUS_INVALID_PARAMETER;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

NTSTATUS UMPseudoValueSet(_In_ PIOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT InputBuffer, _In_ ULONG InputBufferLength)
{
	PVOID data = NULL;
	ULONG dataLength = 0;
	UNICODE_STRING uKeyName;
	UNICODE_STRING uValueName;
	UNICODE_STRING uProcessName;
	PREGISTRY_KEY_RECORD keyRecord = NULL;
	PREGISTRY_VALUE_RECORD valueRecord = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("InputBuffer=0x%p; InputBufferLength=%u", InputBuffer, InputBufferLength);

	if (InputBufferLength >= sizeof(IOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT) &&
		InputBuffer->DataOffset + InputBuffer->DataLength <= InputBufferLength &&
		InputBuffer->KeyNameOffset + InputBuffer->KeyNameLength <= InputBufferLength &&
		InputBuffer->ValueNameOffset + InputBuffer->ValeNameLength <= InputBufferLength &&
		InputBuffer->ProcessNameOffset + InputBuffer->ProcessNameLength <= InputBufferLength) {
		
		uKeyName.Length = InputBuffer->KeyNameLength;
		uKeyName.MaximumLength = uKeyName.Length;
		uKeyName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->KeyNameOffset);
	
		uValueName.Length = InputBuffer->ValeNameLength;
		uValueName.MaximumLength = uValueName.Length;
		uValueName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->ValueNameOffset);

		uProcessName.Length = InputBuffer->ProcessNameLength;
		uProcessName.MaximumLength = uProcessName.Length;
		uProcessName.Buffer = (PWCH)((PUCHAR)InputBuffer + InputBuffer->ProcessNameOffset);
		
		data = (PUCHAR)InputBuffer + InputBuffer->DataOffset;
		dataLength = InputBuffer->DataLength;
		status = KeyRecordGet(&uKeyName, &keyRecord);
		if (NT_SUCCESS(status)) {
			status = KeyRecordGetPseudoValue(keyRecord, &uValueName, &valueRecord);
			if (NT_SUCCESS(status)) {
				status = KeyRecordChangePseudoValue(keyRecord, valueRecord, InputBuffer->ValueType, data, dataLength, InputBuffer->ChangeMode, InputBuffer->DeleteMode, &uProcessName);
				KeyRecordPseudoValueDereference(keyRecord, valueRecord);
			}

			KeyRecordDereference(keyRecord);
		}
	} else status = STATUS_BUFFER_TOO_SMALL;

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}
