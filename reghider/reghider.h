
#ifndef __REGHIDER_H__
#define __REGHIDER_H__

#include <ntifs.h>



#define REGHIDER_CONTROL_DEVICE_NAME           L"\\Device\\RegHider"
#define REGHIDER_SYMBOLIC_LINK_NAME            L"\\DosDevices\\RegHider"


_Function_class_(DRIVER_UNLOAD)
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

_Function_class_(DRIVER_DISPATCH)
NTSTATUS DriverDispatch(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp);



#endif 
