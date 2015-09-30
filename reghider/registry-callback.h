
#ifndef __REGISTRY_CALLBACK_H__
#define __REGISTRY_CALLBACK_H__

#include <ntifs.h>



NTSTATUS RegistryCallbackModuleInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID RegistryCallbackModuleFinit(_In_ PDRIVER_OBJECT DriverObject);



#endif 
