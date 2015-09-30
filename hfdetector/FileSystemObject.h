
#ifndef __FILE_SYSTEM_OBJECT_H__
#define __FILE_SYSTEM_OBJECT_H__


#include <windows.h>
#include <map>
#include "utils.h"


class CFileSystemObject {
public:
	CFileSystemObject(const std::wstring & aFileName, const DWORD aFileAttributes, const ULONG64 aFileId) :
		name_(aFileName), fileAttributes_(aFileAttributes), fileId_(aFileId), foundById_(false), FoundByName_(false) {}
	CFileSystemObject(const FILE_ID_BOTH_DIR_INFORMATION & Record)
		: foundById_(false), FoundByName_(false)
	{
		fileAttributes_ = Record.FileAttributes;
		fileId_ = Record.FileId.QuadPart;
		name_ = std::wstring(Record.FileName, Record.FileNameLength / sizeof(WCHAR));

		return;
	}
	~CFileSystemObject(void)
	{
		for (auto o : children_)
			delete o.second;

		return;
	}
	DWORD FindChildren(HANDLE DirectoryHandle)
	{
		DWORD ret = ERROR_GEN_FAILURE;

		ret = ListDirectory(DirectoryHandle, FileCallback_, this);

		return ret;
	}
	void AddObject(std::wstring & aObjectName, CFileSystemObject *aObject)
	{
		size_t pos = aObjectName.find_first_of(L'\\');

		if (pos == std::wstring::npos) {
			if (children_.find(aObjectName) == children_.end()) {
				aObject->foundById_ = true;
				aObject->name_ = aObjectName;
				children_.insert(std::make_pair(aObject->name_, aObject));
			} else delete aObject;
		} else {
			std::wstring partName = aObjectName.substr(0, pos);
			auto it = children_.find(partName);
			if (it != children_.end()) {
				it->second->AddObject(aObjectName.substr(pos + 1), aObject);
			} else {
				CFileSystemObject *n = 0;
				
				n = new CFileSystemObject(partName, aObject->getFileAttributes(), aObject->getFileId());
				children_.insert(std::make_pair(partName, n));
				n->AddObject(aObjectName.substr(pos + 1), aObject);
			}
		}

		return;
	}
	std::map<std::wstring, CFileSystemObject *>::iterator begin(void) { return children_.begin();  }
	std::map<std::wstring, CFileSystemObject *>::iterator end(void) { return children_.end(); }

	std::wstring getName(void) const { return name_; }
	ULONG getFileAttributes(void) const { return fileAttributes_; }
	ULONG64 getFileId(void) const { return fileId_; }
private:
	std::map<std::wstring, CFileSystemObject *> children_;
	std::wstring name_;
	ULONG fileAttributes_;
	ULONG64 fileId_;
	bool FoundByName_;
	bool foundById_;
	static VOID WINAPI FileCallback_(const PFILE_ID_BOTH_DIR_INFORMATION FileInfo, PVOID Context)
	{
		CFileSystemObject *child = NULL;
		CFileSystemObject *o = (CFileSystemObject *)Context;

		if (FileInfo->FileNameLength > sizeof(WCHAR) || FileInfo->FileName[0] != L'.') {
			if (FileInfo->FileNameLength > 2 * sizeof(WCHAR) || FileInfo->FileName[0] != L'.' || FileInfo->FileName[1] != L'.') {
				child = new CFileSystemObject(*FileInfo);
				child->FoundByName_ = true;
				o->children_.insert(std::make_pair(child->name_, child));
			}
		}

		return;
	}
};






#endif 
