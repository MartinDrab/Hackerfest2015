

#include <windows.h>
#include <stdio.h>
#include "utils.h"
#include "FileSystemObject.h"



static DWORD _OpenDirectoryName(const HANDLE RootHandle, const std::wstring & aName, PHANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = OpenFileRelative(RootHandle, (PWCHAR)aName.c_str(), aName.size()*sizeof(WCHAR), FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, TRUE, TRUE, Handle);
	if (ret == ERROR_SHARING_VIOLATION)
		ret = OpenFileRelative(RootHandle, (PWCHAR)aName.c_str(), aName.size()*sizeof(WCHAR), FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY, FILE_SHARE_WRITE, TRUE, TRUE, Handle);
	
	if (ret == ERROR_SHARING_VIOLATION)
		ret = OpenFileRelative(RootHandle, (PWCHAR)aName.c_str(), aName.size()*sizeof(WCHAR), FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY, FILE_SHARE_READ, TRUE, TRUE, Handle);

	return ret;
}

static DWORD _OpenFileById(const HANDLE VolumeHint, const ULONG64 FileId, PHANDLE Handle)
{
	DWORD ret = ERROR_GEN_FAILURE;
	FILE_ID_DESCRIPTOR fdd;
	HANDLE tmpHandle = NULL;

	fdd.dwSize = sizeof(fdd);
	fdd.Type = FileIdType;
	fdd.FileId.QuadPart = FileId | 0x50000000000000;

	tmpHandle = OpenFileById(VolumeHint, &fdd, FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, FILE_FLAG_BACKUP_SEMANTICS);
	ret = (tmpHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	if (ret == ERROR_SHARING_VIOLATION) {
		tmpHandle = OpenFileById(VolumeHint, &fdd, FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_READ, NULL, FILE_FLAG_BACKUP_SEMANTICS);
		ret = (tmpHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	}

	if (ret == ERROR_SHARING_VIOLATION) {
		tmpHandle = OpenFileById(VolumeHint, &fdd, FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_WRITE, NULL, FILE_FLAG_BACKUP_SEMANTICS);
		ret = (tmpHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	}

	if (ret == ERROR_SUCCESS)
		*Handle = tmpHandle;

	return ret;
}

static DWORD _ScanFileIds(const std::wstring VolumeName, CFileSystemObject & Root)
{
	HANDLE volumeHandle = NULL;
	unsigned int i = 0;
	BY_HANDLE_FILE_INFORMATION bhfi;
	HANDLE fileHandle = INVALID_HANDLE_VALUE;
	DWORD ret = ERROR_GEN_FAILURE;

	volumeHandle = CreateFileW(VolumeName.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ret = (volumeHandle != INVALID_HANDLE_VALUE) ? ERROR_SUCCESS : GetLastError();
	if (ret == ERROR_SUCCESS) {
		for (i = 0; i < 16 * 1024 * 1024; ++i) {
			ret = _OpenFileById(volumeHandle, i, &fileHandle);
			if (ret == ERROR_SUCCESS) {
				POBJECT_NAME_INFORMATION oni = NULL;

				ret = (GetFileInformationByHandle(fileHandle, &bhfi)) ? ERROR_SUCCESS : GetLastError();
				if (ret == ERROR_SUCCESS) {
					ret = QueryObjectName(fileHandle, &oni);
					if (ret == ERROR_SUCCESS) {
						CFileSystemObject *o = NULL;
						std::wstring fileName = std::wstring(oni->Name.Buffer, oni->Name.Length / sizeof(WCHAR)).substr(1);
						size_t pos = fileName.find_first_of(L'\\');

						fileName = fileName.substr(pos + 1);
						pos = fileName.find_first_of(L'\\');
						fileName = fileName.substr(pos + 1);
						o = new CFileSystemObject(L"", bhfi.dwFileAttributes, i);
						Root.AddObject(fileName, o);

						HeapFree(GetProcessHeap(), HEAP_ZERO_MEMORY, oni);
					}
				}

				CloseHandle(fileHandle);
			}
		}

		CloseHandle(volumeHandle);
	}

	return ret;
}

static DWORD _BuildTreeLevel(const HANDLE hDirectory, CFileSystemObject & FSObject)
{
	DWORD ret = ERROR_GEN_FAILURE;

	ret = FSObject.FindChildren(hDirectory);
	if (ret == ERROR_SUCCESS) {
		for (auto it = FSObject.begin(); it != FSObject.end(); ++it) {
			CFileSystemObject *ch = it->second;
			ULONG attr = ch->getFileAttributes();
			
			if ((attr & FILE_ATTRIBUTE_REPARSE_POINT) == 0 && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				HANDLE hChild = NULL;

				ret = _OpenDirectoryName(hDirectory, ch->getName(), &hChild);
				if (ret == ERROR_SUCCESS) {
					ret = _BuildTreeLevel(hChild, *ch);
					CloseHandle(hChild);
				}
			}

//			if (ret != ERROR_SUCCESS)
//				break;
		}
	}

	return ret;
}

void PrintLevel(const std::wstring & aParentName, CFileSystemObject & aObject)
{
	std::wstring fullName = aParentName + L'\\' + aObject.getName();
	printf("0x%x: %S\n", aObject.getFileAttributes(), fullName.c_str());
	for (auto it = aObject.begin(); it != aObject.end(); ++it) {
		PrintLevel(fullName, *it->second);
	}

	return;
}

int main(int arbc, char *argv[])
{
	DWORD ret = ERROR_GEN_FAILURE;
	CFileSystemObject *root = NULL;

	ret = UtilsModuleInit();
	if (ret == ERROR_SUCCESS) {
		ret = AdjustPrivilege(L"SeBackupPrivilege");
		if (ret == ERROR_SUCCESS) {
			ret = AdjustPrivilege(L"SeRestorePrivilege");
			if (ret == ERROR_SUCCESS) {
				HANDLE rootHandle = NULL;
				std::wstring rootPath = L"\\??\\C:\\";


				ret = _OpenDirectoryName(NULL, rootPath, &rootHandle);
				if (ret == ERROR_SUCCESS) {
					root = new CFileSystemObject(rootPath, 0, 0);
					ret = _BuildTreeLevel(rootHandle, *root);
					_ScanFileIds(L"\\\\.\\C:", *root);
					CloseHandle(rootHandle);
					PrintLevel(L"", *root);
				}
			}
		}

		UtilsModuleFinit();
	}

	return (int)ret;
}
