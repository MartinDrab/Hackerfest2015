
#include <ntifs.h>
#include "allocator.h"
#include "preprocessor.h"
#include "reghider-ioctl.h"
#include "key-record.h"
#include "process-db.h"
#include "registry-callback.h"
#include "um-services.h"
#include "reghider.h"



/************************************************************************/
/*                      GLOBAL VARIABLES                                */
/************************************************************************/

static PDEVICE_OBJECT _controlDeviceObject = NULL;

/************************************************************************/
/*                      HELPER FUNCTIONS                                */
/************************************************************************/

_IRQL_requires_(PASSIVE_LEVEL)
static NTSTATUS _DriverInit(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING uDeviceName;
	UNICODE_STRING uSymbolicLinkName;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"", DriverObject, RegistryPath);
	
	RtlInitUnicodeString(&uDeviceName, REGHIDER_CONTROL_DEVICE_NAME);
	status = IoCreateDevice(DriverObject, 0, &uDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &_controlDeviceObject);
	if (NT_SUCCESS(status)) {
		RtlInitUnicodeString(&uSymbolicLinkName, REGHIDER_SYMBOLIC_LINK_NAME);
		status = IoCreateSymbolicLink(&uSymbolicLinkName, &uDeviceName);
		if (NT_SUCCESS(status)) {
			DriverObject->DriverUnload = DriverUnload;
			DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverDispatch;
			DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDispatch;
			DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverDispatch;
		}

		if (!NT_SUCCESS(status)) {
			IoDeleteDevice(_controlDeviceObject);
			_controlDeviceObject = NULL;
		}
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}

static VOID _DriverFinit(_In_ PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING uSymbolicLinkName;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	RtlInitUnicodeString(&uSymbolicLinkName, REGHIDER_SYMBOLIC_LINK_NAME);
	IoDeleteSymbolicLink(&uSymbolicLinkName);
	IoDeleteDevice(_controlDeviceObject);
	_controlDeviceObject = NULL;

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

/************************************************************************/
/*                     PUBLIC FUNCTIONS                                 */
/************************************************************************/

_Function_class_(DRIVER_DISPATCH)
NTSTATUS DriverDispatch(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	PIO_STACK_LOCATION irpStack = NULL;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DeviceObject=0x%p; Irp=0x%p", DeviceObject, Irp);

	irpStack = IoGetCurrentIrpStackLocation(Irp);
	switch (irpStack->MajorFunction) {
		case IRP_MJ_CREATE:
		case IRP_MJ_CLOSE: {
			status = STATUS_SUCCESS;
			if (irpStack->MajorFunction == IRP_MJ_CREATE)
				Irp->IoStatus.Information = FILE_OPENED;
		} break;
		case IRP_MJ_DEVICE_CONTROL: {
			ULONG controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
			ULONG inputBufferLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
			ULONG outputBufferLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
			PVOID inputBuffer = Irp->AssociatedIrp.SystemBuffer;
			PVOID outputBuffer = Irp->AssociatedIrp.SystemBuffer;

			switch (controlCode) {
				case IOCTL_REGHIDER_HIDDEN_KEY_ADD:
					status = UMHiddenKeyAdd((PIOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT)inputBuffer, inputBufferLength);
					break;
				case IOCTL_REGHIDER_HIDDEN_KEY_ENUM:
					outputBuffer = Irp->UserBuffer;
					status = UMHiddenKeyEnum((PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT)outputBuffer, outputBufferLength);
					break;
				case IOCTL_REGHIDER_HIDDEN_KEY_DELETE:
					status = UMHiddenKeyDelete((PIOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT)inputBuffer, inputBufferLength);
					break;
				case IOCTL_REGHIDER_PSEUDO_VALUE_ADD:
					status = UMPseudoValueAdd((PIOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT)inputBuffer, inputBufferLength);
					break;
				case IOCTL_REGHIDER_PSEUDO_VALUE_ENUM:
					outputBuffer = Irp->UserBuffer;
					status = UMPseudoValueEnum((PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT)outputBuffer, outputBufferLength);
					break;
				case IOCTL_REGHIDER_PSEUDO_VALUE_DELETE:
					status = UMPseudoValueDelete((PIOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT)inputBuffer, inputBufferLength);
					break;
				case IOCTL_REGHIDER_PSUEDO_VALUE_SET:
					status = UMPseudoValueSet((PIOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT)inputBuffer, inputBufferLength);
					break;
				default:
					status = STATUS_INVALID_DEVICE_REQUEST;
					break;
			}
		} break;
		default: {
			status = STATUS_INVALID_DEVICE_REQUEST;
		} break;
	}

	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}


_Function_class_(DRIVER_UNLOAD)
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p", DriverObject);

	_DriverFinit(DriverObject);
	RegistryCallbackModuleFinit(DriverObject);
	ProcessDBModuleFinit(DriverObject);
	KeyRecordModuleFInit(DriverObject);
	DebugAllocatorModuleFinit();

	DEBUG_EXIT_FUNCTION_VOID();
	return;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	DEBUG_ENTER_FUNCTION("DriverObject=0x%p; RegistryPath=\"%wZ\"", DriverObject, RegistryPath);

	status = DebugAllocatorModuleInit();
	if (NT_SUCCESS(status)) {
		status = KeyRecordModuleInit(DriverObject, RegistryPath);
		if (NT_SUCCESS(status)) {
			status = ProcessDBModuleInit(DriverObject, RegistryPath);
			if (NT_SUCCESS(status)) {
				status = RegistryCallbackModuleInit(DriverObject, RegistryPath);
				if (NT_SUCCESS(status)) {
					status = _DriverInit(DriverObject, RegistryPath);
					if (!NT_SUCCESS(status))
						RegistryCallbackModuleFinit(DriverObject);
				}
			
				if (!NT_SUCCESS(status))
					ProcessDBModuleFinit(DriverObject);
			}

			if (!NT_SUCCESS(status))
				KeyRecordModuleFInit(DriverObject);
		}

		if (!NT_SUCCESS(status))
			DebugAllocatorModuleFinit();
	}

	DEBUG_EXIT_FUNCTION("0x%x", status);
	return status;
}
