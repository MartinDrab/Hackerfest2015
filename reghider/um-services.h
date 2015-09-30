
#ifndef __UM_SERVICES_H__
#define __UM_SERVICES_H__

#include <ntifs.h>
#include "reghider-ioctl.h"



NTSTATUS UMHiddenKeyAdd(_In_ PIOCTL_REGHIDER_HIDDEN_KEY_ADD_INPUT InputBuffer, _In_ ULONG InputBufferLength);
NTSTATUS UMHiddenKeyEnum(_Out_ PIOCTL_REGHIDER_HIDDEN_KEY_ENUM_OUTPUT OutputBuffer, _In_ ULONG OutputBufferLength);
NTSTATUS UMHiddenKeyDelete(_In_ PIOCTL_REGHIDER_HIDDEN_KEY_DELETE_INPUT InputBuffer, _In_ ULONG InputBufferLength);

NTSTATUS UMPseudoValueAdd(_In_ PIOCTL_REGHIDER_PSEUDO_VALUE_ADD_INPUT InputBuffer, _In_ ULONG InputBufferLength);
NTSTATUS UMPseudoValueEnum(_Out_ PIOCTL_REGHIDER_PSEUDO_VALUE_ENUM_OUTPUT OutputBuffer, _In_ ULONG OutputBufferLength);
NTSTATUS UMPseudoValueDelete(_In_ PIOCTL_REGHIDER_PSEUDO_VALUE_DELETE_INPUT InputBuffer, _In_ ULONG InputBufferLength);
NTSTATUS UMPseudoValueSet(_In_ PIOCTL_REGHIDER_PSUEDO_VALUE_SET_INPUT InputBuffer, _In_ ULONG InputBufferLength);




#endif 
