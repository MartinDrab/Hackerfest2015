
#include <windows.h>
#include "reghider-types.h"
#include "reghider-ioctl.h"
#include "libreghider.h"


/************************************************************************/
/*                          GLOBAL VARIABLES                            */
/************************************************************************/

static HANDLE _driverHandle = INVALID_HANDLE_VALUE;

/************************************************************************/
/*                          HELPER FUNCTIONS                            */
/************************************************************************/

static DWORD _SynchronousWriteIOCTL(ULONG ControlCode, PVOID InputBuffer, ULONG InputBufferLength)
{
	DWORD dummy = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	ret = (DeviceIoControl(_driverHandle, ControlCode, InputBuffer, InputBufferLength, NULL, 0, &dummy, NULL)) ? ERROR_SUCCESS : GetLastError();

	return ret;
}

static DWORD _SynchronousReadIOCTL(ULONG ControlCode, PVOID OutputBuffer, ULONG OutputBufferLength)
{
	DWORD dummy = 0;
	DWORD ret = ERROR_GEN_FAILURE;

	ret = (DeviceIoControl(_driverHandle, ControlCode, NULL, 0, OutputBuffer, OutputBufferLength, &dummy, NULL)) ? ERROR_SUCCESS : GetLastError();

	return ret;
}

static DWORD _SynchronousReadIOCTLVariableLength(ULONG ControlCode, PVOID *Buffer, PULONG BufferLength)
{
	ULONG tmpLength = 0;
	PVOID tmpBuffer = NULL;
	DWORD ret = ERROR_GEN_FAILURE;

	tmpLength = *BufferLength;
	do {
		tmpBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, tmpLength);
		if (tmpBuffer != NULL) {
			ret = _SynchronousReadIOCTL(ControlCode, tmpBuffer, tmpLength);
			if (ret != ERROR_SUCCESS) {
				HeapFree(GetProcessHeap(), 0, tmpBuffer);
				if (ret == ERROR_INSUFFICIENT_BUFFER)
					tmpLength += 2;
			}
		} else ret = ERROR_NOT_ENOUGH_MEMORY;
	} while (ret == ERROR_INSUFFICIENT_BUFFER);

	if (ret == ERROR_SUCCESS) {
		*BufferLength = tmpLength;
		*Buffer = tmpBuffer;
	}

	return ret;
}

static DWORD _CreateHiddenKeyRecord(PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT Entry, PREGHIDER_HIDDEN_KEY_RECORD Record)
{
	DWORD ret = ERROR_GEN_FAILURE;
	
	Record->KeyName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Entry->KeyNameLength + sizeof(WCHAR));
	if (Record->KeyName != NULL) {
		memcpy(Record->KeyName, Entry->KeyName, Entry->KeyNameLength);
		ret = ERROR_SUCCESS;
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

static VOID _FreeHiddenKeyRecord(PREGHIDER_HIDDEN_KEY_RECORD Record)
{
	HeapFree(GetProcessHeap(), 0, Record->KeyName);

	return;
}

static DWORD _CreatePseudoValueRecord(PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT Entry, PREGHIDER_PSEUDO_VALUE_RECORD Record)
{
	DWORD ret = ERROR_GEN_FAILURE;

	Record->KeyName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Entry->KeyNameLength + sizeof(WCHAR));
	if (Record->KeyName != NULL) {
		ret = ERROR_SUCCESS;
		memcpy(Record->KeyName, (PUCHAR)Entry + Entry->KeyNameOffset, Entry->KeyNameLength);
		Record->ValueName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Entry->NameLength + sizeof(WCHAR));
		if (Record->ValueName != NULL) {
			memcpy(Record->ValueName, (PUCHAR)Entry + Entry->NameOffset, Entry->NameLength);
			Record->ProcessName = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Entry->ProcessNameLength + sizeof(WCHAR));
			if (Record->ProcessName != NULL) {
				memcpy(Record->ProcessName, (PUCHAR)Entry + Entry->ProcessNameOffset, Entry->ProcessNameLength);
				Record->DataLength = Entry->DataLength;
				Record->ValueType = Entry->Valuetype;
				Record->ChangeMode = Entry->ChangeMode;
				Record->DeleteMode = Entry->DeleteMode;
				if (Record->DataLength > 0) {
					Record->Data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Entry->DataLength);
					if (Record->Data != NULL) {
						memcpy(Record->Data, (PUCHAR)Entry + Entry->DataOffset, Entry->DataLength);
					} else ret = ERROR_NOT_ENOUGH_MEMORY;
				}

				if (ret != ERROR_SUCCESS)
					HeapFree(GetProcessHeap(), 0, Record->ProcessName);
			} else ret = ERROR_NOT_ENOUGH_MEMORY;

			if (ret != ERROR_SUCCESS)
				HeapFree(GetProcessHeap(), 0, Record->ValueName);
		} else ret = ERROR_NOT_ENOUGH_MEMORY;

		if (ret != ERROR_SUCCESS)
			HeapFree(GetProcessHeap(), 0, Record->KeyName);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

static VOID _FreePseudoValueRecord(PREGHIDER_PSEUDO_VALUE_RECORD Record)
{
	if (Record->KeyName != NULL)
		HeapFree(GetProcessHeap(), 0, Record->KeyName);

	if (Record->ValueName != NULL)
		HeapFree(GetProcessHeap(), 0, Record->ValueName);

	if (Record->Data != NULL)
		HeapFree(GetProcessHeap(), 0, Record->Data);

	if (Record->ProcessName != NULL)
		HeapFree(GetProcessHeap(), 0, Record->ProcessName);

	return;
}

/************************************************************************/
/*                    PUBLIC FUNCTIONS                                  */
/************************************************************************/

/*****************/
/* PSEUDO VALUES */
/*****************/

DWORD LibRegHiderPseudoValueAdd(PWCHAR KeyName, PWCHAR ValueName, ULONG Valuetype, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName)
{
	USHORT keyNameLen = 0;
	USHORT valueNameLen = 0;
	USHORT processNameLen = 0;
	ULONG inputLen = sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT);
	PIOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT input = NULL;
	DWORD ret = ERROR_GEN_FAILURE;

	keyNameLen = (USHORT)wcslen(KeyName)*sizeof(WCHAR);
	valueNameLen = (ValueName != NULL) ? (USHORT)wcslen(ValueName)*sizeof(WCHAR) : 0;
	processNameLen = (ProcessName != NULL) ? (USHORT)wcslen(ProcessName)*sizeof(WCHAR) : 0;
	inputLen += keyNameLen + valueNameLen + processNameLen + DataLength;

	input = (PIOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
	if (input != NULL) {
		input->KeyNameOffset = sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT);
		input->KeyNameLength = keyNameLen;
		memcpy((PUCHAR)input + input->KeyNameOffset, KeyName, input->KeyNameLength);

		input->NameOffset = input->KeyNameOffset + input->KeyNameLength;
		input->NameLength = valueNameLen;
		memcpy((PUCHAR)input + input->NameOffset, ValueName, input->NameLength);

		input->ProcessNameOffset = input->NameOffset + input->NameLength;
		input->ProcessNameLength = processNameLen;
		memcpy((PUCHAR)input + input->ProcessNameOffset, ProcessName, input->ProcessNameLength);

		input->DataOffset = input->ProcessNameOffset + input->ProcessNameLength;
		input->DataLength = DataLength;
		memcpy((PUCHAR)input + input->DataOffset, Data, input->DataLength);

		input->ValueType = Valuetype;
		input->ChangeMode = ChangeMode;
		input->DeleteMode = DeleteMode;
		ret = _SynchronousWriteIOCTL(IOCTL_REGHIDER_PSEUDO_VALUE_ADD, input, inputLen);
		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

DWORD LibRegHiderPseudoValueDelete(PWCHAR KeyName, PWCHAR ValueName)
{
	ULONG inputLen = 0;
	USHORT keyNameLen = 0;
	USHORT valueNameLen = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	PIOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT input = NULL;

	keyNameLen = (USHORT)wcslen(KeyName)*sizeof(WCHAR);
	valueNameLen = (ValueName != NULL) ? (USHORT)wcslen(ValueName)*sizeof(WCHAR) : 0;
	inputLen = sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT) + keyNameLen + valueNameLen;

	input = (PIOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
	if (input != NULL) {
		input->KeyNameOffset = sizeof(IOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT);
		input->KeyNameLength = keyNameLen;
		memcpy((PUCHAR)input + input->KeyNameOffset, KeyName, input->KeyNameLength);

		input->ValueNameOffset = input->KeyNameOffset + input->KeyNameLength;
		input->ValueNameLength = valueNameLen;
		memcpy((PUCHAR)input + input->ValueNameOffset, ValueName, input->ValueNameLength);

		ret = _SynchronousWriteIOCTL(IOCTL_REGHIDER_PSEUDO_VALUE_DELETE, input, inputLen);
		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

DWORD LibRegHiderPseudoValuesEnum(REGHIDER_PSEUDO_VALUE_CALLBACK *Callback, PVOID Context)
{
	BOOL continueEnumeration = FALSE;
	DWORD ret = ERROR_GEN_FAILURE;
	ULONG outputSize = 128;
	PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT output = NULL;

	ret = _SynchronousReadIOCTLVariableLength(IOCTL_REGHIDER_PSEUDO_VALUE_ENUM, (PVOID *)&output, &outputSize);
	if (ret == ERROR_SUCCESS) {
		PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT tmp = output;
		REGHIDER_PSEUDO_VALUE_RECORD valueRecord;

		ret = _CreatePseudoValueRecord(tmp, &valueRecord);
		if (ret == ERROR_SUCCESS) {
			continueEnumeration = Callback(&valueRecord, Context);
			_FreePseudoValueRecord(&valueRecord);
			while (continueEnumeration && ret == ERROR_SUCCESS && tmp->NextEntryOffset > 0) {
				tmp = (PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT)((PUCHAR)tmp + tmp->NextEntryOffset);
				ret = _CreatePseudoValueRecord(tmp, &valueRecord);
				if (ret == ERROR_SUCCESS) {
					continueEnumeration = Callback(&valueRecord, Context);
					_FreePseudoValueRecord(&valueRecord);
				}
			}

			if (!continueEnumeration)
				ret = ERROR_CONNECTION_ABORTED;
		}

		HeapFree(GetProcessHeap(), 0, output);
	}

	if (ret == ERROR_NO_MORE_ITEMS)
		ret = ERROR_SUCCESS;

	return ret;
}

DWORD LibRegHiderPseudoValueSet(PWCHAR KeyName, PWCHAR ValueName, ULONG ValueType, PVOID Data, ULONG DataLength, ERegistryValueOpMode DeleteMode, ERegistryValueOpMode ChangeMode, PWCHAR ProcessName)
{
	USHORT keyNameLen = 0;
	USHORT valueNameLen = 0;
	USHORT processnameLen = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	ULONG inputLen = sizeof(IOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT);
	PIOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT input = NULL;

	keyNameLen = (KeyName != NULL) ? (USHORT)wcslen(KeyName)*sizeof(WCHAR) : 0;
	valueNameLen = (ValueName != NULL) ? (USHORT)wcslen(ValueName)*sizeof(WCHAR) : 0;
	processnameLen = (ProcessName != NULL) ? (USHORT)wcslen(ProcessName)*sizeof(WCHAR) : 0;
	inputLen += (keyNameLen + valueNameLen + processnameLen + DataLength);
	input = (PIOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
	if (input != NULL) {
		input->ValueType = ValueType;
		input->ChangeMode = ChangeMode;
		input->DeleteMode = DeleteMode;

		input->KeyNameLength = keyNameLen;
		input->KeyNameOffset = sizeof(IOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT);
		memcpy((PUCHAR)input + input->KeyNameOffset, KeyName, keyNameLen);

		input->ValeNameLength = valueNameLen;
		input->ValueNameOffset = input->KeyNameOffset + keyNameLen;
		memcpy((PUCHAR)input + input->ValueNameOffset, ValueName, valueNameLen);

		input->ProcessNameLength = processnameLen;
		input->ProcessNameOffset = input->ValueNameOffset + valueNameLen;
		memcpy((PUCHAR)input + input->ProcessNameOffset, ProcessName, processnameLen);

		input->DataLength = DataLength;
		input->DataOffset = input->ProcessNameOffset + processnameLen;
		memcpy((PUCHAR)input + input->DataOffset, Data, DataLength);
		ret = _SynchronousWriteIOCTL(IOCTL_REGHIDER_PSUEDO_VALUE_SET, input, inputLen);
		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}


/***************/
/* HIDDEN KEYS */
/***************/

DWORD LibRegHiderHiddenKeyAdd(PWCHAR KeyName)
{
	ULONG inputLen = 0;
	USHORT keyNameLen = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	PIOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT input = NULL;

	keyNameLen = (USHORT)wcslen(KeyName)*sizeof(WCHAR);
	inputLen = sizeof(IOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT) + keyNameLen;
	input = (PIOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
	if (input != NULL) {
		input->KeyNameLength = keyNameLen;
		memcpy(input->KeyName, KeyName, input->KeyNameLength);
		ret = _SynchronousWriteIOCTL(IOCTL_REGHIDER_HIDDEN_KEY_ADD, input, inputLen);
		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

DWORD LibRegHiderHiddenKeyDelete(PWCHAR KeyName)
{
	ULONG inputLen = 0;
	USHORT keyNameLen = 0;
	DWORD ret = ERROR_GEN_FAILURE;
	PIOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT input = NULL;

	keyNameLen = (USHORT)wcslen(KeyName)*sizeof(WCHAR);
	inputLen = sizeof(IOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT) + keyNameLen;
	input = (PIOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, inputLen);
	if (input != NULL) {
		input->KeyNameLength = keyNameLen;
		memcpy(input->KeyName, KeyName, input->KeyNameLength);
		ret = _SynchronousWriteIOCTL(IOCTL_REGHIDER_HIDDEN_KEY_DELETE, input, inputLen);
		HeapFree(GetProcessHeap(), 0, input);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

DWORD LibRegHiderHiddenKeysEnum(REGHIDER_HIDDEN_KEY_CALLBACK *Callback, PVOID Context)
{
	BOOL continueEnumeration = FALSE;
	DWORD ret = ERROR_GEN_FAILURE;
	ULONG outputSize = 128;
	PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT output = NULL;

	ret = _SynchronousReadIOCTLVariableLength(IOCTL_REGHIDER_HIDDEN_KEY_ENUM, (PVOID *)&output, &outputSize);
	if (ret == ERROR_SUCCESS) {
		PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT tmp = output;
		REGHIDER_HIDDEN_KEY_RECORD keyRecord;

		ret = _CreateHiddenKeyRecord(tmp, &keyRecord);
		if (ret == ERROR_SUCCESS) {
			continueEnumeration = Callback(&keyRecord, Context);
			_FreeHiddenKeyRecord(&keyRecord);
			while (continueEnumeration && ret == ERROR_SUCCESS && tmp->NextEntryOffset > 0) {
				tmp = (PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT)((PUCHAR)tmp + tmp->NextEntryOffset);
				ret = _CreateHiddenKeyRecord(tmp, &keyRecord);
				if (ret == ERROR_SUCCESS) {
					continueEnumeration = Callback(&keyRecord, Context);
					_FreeHiddenKeyRecord(&keyRecord);
				}
			}

			if (!continueEnumeration)
				ret = ERROR_CONNECTION_ABORTED;
		}

		HeapFree(GetProcessHeap(), 0, output);
	}

	if (ret == ERROR_NO_MORE_ITEMS)
		ret = ERROR_SUCCESS;

	return ret;
}


/************************************************************************/
/*                        INITIALIZATION AND FINALIZATION               */
/************************************************************************/

DWORD LibRegHiderInit(VOID)
{
	DWORD ret = ERROR_GEN_FAILURE;

	_driverHandle = CreateFileW(REGHIDER_USERMODE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ret = (_driverHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();

	return ret;
}

VOID LibRegHiderFinit(VOID)
{
	if (_driverHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(_driverHandle);
		_driverHandle = NULL;
	}

	return;
}