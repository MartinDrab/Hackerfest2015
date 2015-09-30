
#ifndef __HF_DETECT_H__
#define __HF_DETECT_H__

#include <windows.h>
#include <Winternl.h>


typedef struct _FILE_ID_BOTH_DIR_INFORMATION {
	ULONG         NextEntryOffset;
	ULONG         FileIndex;
	LARGE_INTEGER CreationTime;
	LARGE_INTEGER LastAccessTime;
	LARGE_INTEGER LastWriteTime;
	LARGE_INTEGER ChangeTime;
	LARGE_INTEGER EndOfFile;
	LARGE_INTEGER AllocationSize;
	ULONG         FileAttributes;
	ULONG         FileNameLength;
	ULONG         EaSize;
	CCHAR         ShortNameLength;
	WCHAR         ShortName[12];
	LARGE_INTEGER FileId;
	WCHAR         FileName[1];
} FILE_ID_BOTH_DIR_INFORMATION, *PFILE_ID_BOTH_DIR_INFORMATION;


#define FileIdBothDirectoryInformation							37
#define FileIdFullDirectoryInformation							38

typedef NTSTATUS (NTAPI NTQUERYDIRECTORYFILE)(
	_In_     HANDLE                 FileHandle,
	_In_opt_ HANDLE                 Event,
	_In_opt_ PIO_APC_ROUTINE        ApcRoutine,
	_In_opt_ PVOID                  ApcContext,
	_Out_    PIO_STATUS_BLOCK       IoStatusBlock,
	_Out_    PVOID                  FileInformation,
	_In_     ULONG                  Length,
	_In_     ULONG FileInformationClass,
	_In_     BOOLEAN                ReturnSingleEntry,
	_In_opt_ PUNICODE_STRING        FileName,
	_In_     BOOLEAN                RestartScan);

typedef NTSTATUS (NTAPI NTOPENFILE)(
	_Out_ PHANDLE            FileHandle,
	_In_  ACCESS_MASK        DesiredAccess,
	_In_  POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_ PIO_STATUS_BLOCK   IoStatusBlock,
	_In_  ULONG              ShareAccess,
	_In_  ULONG              OpenOptions);

typedef ULONG (WINAPI RTLNTSTATUSTODOSERROR)(_In_ NTSTATUS Status);

// Mozne druhy informaci, ktere lze ziskat pres rutinu NtQueryObject
typedef enum _PRIVATE_OBJECT_INFORMATION_CLASS {
	// Zakladni informace
	PrivateObjectBasicInformation,
	// Jmeno objektu
	PrivateObjectNameInformation,
	// Informace o typu objektu
	PrivateObjectTypeInformation,
	// Informace o vlastnostech a pocet objektu vsech typu
	PrivateObjectTypesInformation,
	// Atributy handle (P, I)
	PrivateObjectHandleFlagInformation,
	PrivateObjectSessionInformation,
} PRIVATE_OBJECT_INFORMATION_CLASS;

// Zakladni informace o objektu
typedef struct _PRIVATE_OBJECT_BASIC_INFORMATION {
	ULONG Attributes;
	ACCESS_MASK GrantedAccess;
	ULONG HandleCount;
	ULONG PointerCount;
	ULONG PagedPoolCharge;
	ULONG NonPagedPoolCharge;
	ULONG Reserved[3];
	ULONG NameInfoSize;
	ULONG TypeInfoSize;
	ULONG SecurityDescriptorSize;
	// Cas vytvoreni, platny pouze u symbolickych odkazu
	LARGE_INTEGER CreationTime;
} PRIVATE_OBJECT_BASIC_INFORMATION, *PPRIVATE_OBJECT_BASIC_INFORMATION;

// Jmeno objektu
typedef struct _OBJECT_NAME_INFORMATION {               // ntddk wdm nthal
	UNICODE_STRING Name;                                // ntddk wdm nthal
} OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;   // ntddk wdm nthal


typedef NTSTATUS (NTAPI NTQUERYOBJECT)(
	_In_opt_  HANDLE Handle,
	_In_      PRIVATE_OBJECT_INFORMATION_CLASS	ObjectInformationClass,
	_Out_opt_ PVOID	ObjectInformation,
	_In_      ULONG	ObjectInformationLength,
	_Out_opt_ PULONG ReturnLength);



typedef VOID(WINAPI FILE_CALLBACK)(const PFILE_ID_BOTH_DIR_INFORMATION FileInfo, PVOID Context);



DWORD AdjustPrivilege(const PWCHAR PrivilegeName);
DWORD ListDirectory(HANDLE hDirectory, FILE_CALLBACK *Callback, PVOID Context);
DWORD OpenFileRelative(const HANDLE hDirectory, const PWCHAR FileName, const USHORT FileNameLength, const DWORD DesiredAccess, const DWORD ShareMode, const BOOLEAN Directory, const BOOLEAN BackupIntent, PHANDLE FileHandle);
DWORD QueryObjectName(HANDLE Object, POBJECT_NAME_INFORMATION *Name);

DWORD UtilsModuleInit(VOID);
VOID UtilsModuleFinit(VOID);



#endif 
