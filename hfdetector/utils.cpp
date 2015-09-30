
#include <windows.h>
#include <Winternl.h>
#include "utils.h"



/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static NTOPENFILE *_NtOpenFile = NULL;
static NTQUERYDIRECTORYFILE *_NtQueryDirectoryFile = NULL;
static RTLNTSTATUSTODOSERROR *_RtlNtStatusToDosError = NULL;
static NTQUERYOBJECT *_NtQueryObject = NULL;

/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/

DWORD AdjustPrivilege(const PWCHAR PrivilegeName)
{
	HANDLE hToken = NULL;
	LUID privilegeValue;
	ULONG retLength = 0;
	TOKEN_PRIVILEGES newPrivs;
	TOKEN_PRIVILEGES oldPrivs;
	DWORD ret = ERROR_GEN_FAILURE;

	if (LookupPrivilegeValueW(NULL, PrivilegeName, &privilegeValue)) {
		if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken)) {
			newPrivs.PrivilegeCount = 1;
			newPrivs.Privileges[0].Luid = privilegeValue;
			newPrivs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			ret = (AdjustTokenPrivileges(hToken, FALSE, &newPrivs, sizeof(newPrivs), &oldPrivs, &retLength)) ? ERROR_SUCCESS : GetLastError();
			CloseHandle(hToken);
		} else ret = GetLastError();
	} else ret = GetLastError();

	return ret;
}

DWORD ListDirectory(HANDLE hDirectory, FILE_CALLBACK *Callback, PVOID Context)
{
	DWORD ret = ERROR_GEN_FAILURE;
	NTSTATUS status = 0xC0000001;
	ULONG bdiLen = 0;
	IO_STATUS_BLOCK iosb;
	PFILE_ID_BOTH_DIR_INFORMATION bdi = NULL;
	PFILE_ID_BOTH_DIR_INFORMATION tmp = NULL;

	bdiLen = 256 * sizeof(WCHAR) + 30 * sizeof(FILE_ID_BOTH_DIR_INFORMATION);
	bdi = (PFILE_ID_BOTH_DIR_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bdiLen);
	if (bdi != NULL) {
		status = _NtQueryDirectoryFile(hDirectory, NULL, NULL, NULL, &iosb, bdi, bdiLen, FileIdBothDirectoryInformation, FALSE, NULL, TRUE);
		if (NT_SUCCESS(status) || status == 0x80000005) {
			do {
				tmp = bdi;
				Callback(tmp, Context);
				while (tmp->NextEntryOffset > 0) {
					tmp = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)tmp + tmp->NextEntryOffset);
					Callback(tmp, Context);
				}

				status = _NtQueryDirectoryFile(hDirectory, NULL, NULL, NULL, &iosb, bdi, bdiLen, FileIdBothDirectoryInformation, FALSE, NULL, FALSE);
			} while (NT_SUCCESS(status) || status == 0x80000005);
		}

		if (status == 0x80000006)
			status = 0;

		if (NT_SUCCESS(status))
			ret = ERROR_SUCCESS;

		if (!NT_SUCCESS(status))
			ret = _RtlNtStatusToDosError(status);

		HeapFree(GetProcessHeap(), 0, bdi);
	} else ret = ERROR_NOT_ENOUGH_MEMORY;

	return ret;
}

DWORD OpenFileRelative(const HANDLE hDirectory, const PWCHAR FileName, const USHORT FileNameLength, const DWORD DesiredAccess, const DWORD ShareMode, const BOOLEAN Directory, const BOOLEAN BackupIntent, PHANDLE FileHandle)
{
	IO_STATUS_BLOCK iosb;
	OBJECT_ATTRIBUTES oa;
	UNICODE_STRING uFileName;
	NTSTATUS status = 0xC0000001;
	DWORD ret = ERROR_GEN_FAILURE;

	uFileName.Length = FileNameLength;
	uFileName.MaximumLength = uFileName.Length;
	uFileName.Buffer = FileName;
	InitializeObjectAttributes(&oa, &uFileName, OBJ_CASE_INSENSITIVE, hDirectory, NULL);
	status = _NtOpenFile(FileHandle, DesiredAccess | SYNCHRONIZE, &oa, &iosb, ShareMode, (Directory ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE) | (BackupIntent ? FILE_OPEN_FOR_BACKUP_INTENT  : 0) | FILE_SYNCHRONOUS_IO_NONALERT);
	if (NT_SUCCESS(status))
		ret = ERROR_SUCCESS;

	if (!NT_SUCCESS(status))
		ret = _RtlNtStatusToDosError(status);

	return ret;
}

DWORD QueryObjectName(HANDLE Object, POBJECT_NAME_INFORMATION *Name)
{
	ULONG retLength = 0;
	POBJECT_NAME_INFORMATION oni = NULL;
	DWORD ret = ERROR_GEN_FAILURE;
	NTSTATUS status = 0xC0000001;

	status = _NtQueryObject(Object, PrivateObjectNameInformation, NULL, 0, &retLength);
	if (status == 0xC0000004) {
		retLength += sizeof(OBJECT_NAME_INFORMATION);
		oni = (POBJECT_NAME_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retLength);
		if (oni != NULL) {
			status = _NtQueryObject(Object, PrivateObjectNameInformation, oni, retLength, &retLength);
			if (NT_SUCCESS(status)) {
				*Name = oni;
				ret = ERROR_SUCCESS;
			}
			
			if (!NT_SUCCESS(status))
				HeapFree(GetProcessHeap(), 0, oni);
		}
	}

	if (!NT_SUCCESS(status))
		ret = _RtlNtStatusToDosError(status);

	return ret;
}

/************************************************************************/
/*               INITIALIZATION AND FINALIZATION                        */
/************************************************************************/

DWORD UtilsModuleInit(VOID)
{
	HMODULE hntdll = NULL;
	DWORD ret = ERROR_GEN_FAILURE;

	hntdll = GetModuleHandleW(L"ntdll.dll");
	if (hntdll != NULL) {
		_RtlNtStatusToDosError = (RTLNTSTATUSTODOSERROR *)GetProcAddress(hntdll, "RtlNtStatusToDosError");
		if (_RtlNtStatusToDosError != NULL)
			_NtQueryObject = (NTQUERYOBJECT *)GetProcAddress(hntdll, "NtQueryObject");

		if (_NtQueryObject != NULL)
			_NtOpenFile = (NTOPENFILE *)GetProcAddress(hntdll, "NtOpenFile");

		if (_NtOpenFile != NULL)
			_NtQueryDirectoryFile = (NTQUERYDIRECTORYFILE *)GetProcAddress(hntdll, "NtQueryDirectoryFile");
	
		ret = (_NtQueryDirectoryFile != NULL) ? ERROR_SUCCESS : GetLastError();
	} else ret = GetLastError();

	return ret;
}

VOID UtilsModuleFinit(VOID)
{
	_NtQueryDirectoryFile = NULL;
	_NtOpenFile = NULL;
	_NtQueryObject = NULL;
	_RtlNtStatusToDosError = NULL;

	return;
}
