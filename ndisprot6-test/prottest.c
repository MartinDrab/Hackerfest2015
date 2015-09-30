//
// UIOTEST.C
//
// Test program for ndisprot.sys
//
// usage: UIOTEST [options] <devicename>
//
// options:
//        -e: Enumerate devices
//        -r: Read
//        -w: Write (default)
//        -l <length>: length of each packet (default: %d)\n", PacketLength
//        -n <count>: number of packets (defaults to infinity)
//        -m <MAC address> (defaults to local MAC)
//

#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4127)   // conditional expression is constant

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <malloc.h>

#include <winerror.h>
#include <winsock.h>

#include <ntddndis.h>
#include "protuser.h"
#include "ip-headers.h"

// this is needed to prevent comppiler from complaining about 
// pragma prefast statements below
#ifndef _PREFAST_
    #pragma warning(disable:4068)
#endif

#ifndef NDIS_STATUS
#define NDIS_STATUS     ULONG
#endif

#if DBG
#define DEBUGP(stmt)    printf stmt
#else
#define DEBUGP(stmt)
#endif

#define PRINTF(stmt)    printf stmt

#ifndef MAC_ADDR_LEN
#define MAC_ADDR_LEN                    6
#endif

#define MAX_NDIS_DEVICE_NAME_LEN        256

CHAR            NdisProtDevice[] = "\\\\.\\NdisProt";
CHAR *          pNdisProtDevice = &NdisProtDevice[0];

BOOLEAN         DoEnumerate = FALSE;
BOOLEAN         DoReads = FALSE;
INT             NumberOfPackets = -1;
UCHAR           SrcMacAddr[MAC_ADDR_LEN];
UCHAR           DstMacAddr[MAC_ADDR_LEN];
BOOLEAN         bDstMacSpecified = FALSE;
CHAR *          pNdisDeviceName = "JUNK";
USHORT          EthType = 0x0800;
PCHAR			SourceIPAddress = "0.0.0.0";
PCHAR			DestIPAddress = "0.0.0.0";
PCHAR			PacketData = "TESTTESTTEST";


VOID PrintUsage()
{
	PRINTF(("usage: PROTTEST [options] <devicename>\n"));
	PRINTF(("options:\n"));
	PRINTF(("       -e: Enumerate devices\n"));
	PRINTF(("       -n <count>: number of packets (defaults to infinity)\n"));
	PRINTF(("       -m <MAC address> (defaults to local MAC)\n"));
	PRINTF(("       -s <IP> source IP address\n"));
	PRINTF(("       -d <IP> destination IP address\n"));
	PRINTF(("        -t <String> Data to send within the ICMP packets\n"));

	return;
}

BOOL GetOptions(INT argc,__in_ecount(argc) CHAR          *argv[])
{
    BOOL        bOkay;
    INT         i, j, increment;
    CHAR        *pOption;
    ULONG       DstMacAddrUlong[MAC_ADDR_LEN];
    INT         RetVal;

    bOkay = TRUE;

    do
    {
        if (argc < 2)
        {
            PRINTF(("Missing <devicename> argument\n"));
            bOkay = FALSE;
            break;
        }

        i = 1;
#pragma prefast(suppress: 12004, "argc is always >= 1")           
        while (i < argc)
        {
            increment = 1;
            pOption = argv[i];

            if ((*pOption == '-') || (*pOption == '/'))
            {
                pOption++;
                if (*pOption == '\0')
                {
                    DEBUGP(("Badly formed option\n"));
                    return (FALSE);
                }
            }
            else
            {
                break;
            }

            switch (*pOption)
            {
                case 'e':
                    DoEnumerate = TRUE;
                    break;
				case 's': {
					if (i + 1 < argc - 1) {
						SourceIPAddress = argv[i + 1];
						increment = 2;
					}
				} break;
				case 'd': {
					if (i + 1 < argc - 1) {
						DestIPAddress = argv[i + 1];
						increment = 2;
					}
				} break;
				case 't': {
					if (i + 1 < argc - 1) {
						PacketData = argv[i + 1];
						increment = 2;
					}
				} break;
                case 'n':
                    if (i+1 < argc-1) {
                        RetVal = atoi(argv[i+1]);
                        if (RetVal != 0)
                        {
                            NumberOfPackets = RetVal;
                            DEBUGP((" Option: NumberOfPackets = %d\n", NumberOfPackets));
                            increment = 2;
                            break;
                        }
                    }
                    PRINTF(("Option n needs NumberOfPackets parameter\n"));
                    return (FALSE);
                case 'm':

                    if (i+1 < argc-1)
                    {
                        RetVal = sscanf(argv[i+1], "%2x:%2x:%2x:%2x:%2x:%2x",
                                    &DstMacAddrUlong[0],
                                    &DstMacAddrUlong[1],
                                    &DstMacAddrUlong[2],
                                    &DstMacAddrUlong[3],
                                    &DstMacAddrUlong[4],
                                    &DstMacAddrUlong[5]);

                        if (RetVal == 6)
                        {
                            for (j = 0; j < MAC_ADDR_LEN; j++)
                            {
                                DstMacAddr[j] = (UCHAR)DstMacAddrUlong[j];
                            }
    
                            DEBUGP((" Option: Dest MAC Addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
                                DstMacAddr[0],
                                DstMacAddr[1],
                                DstMacAddr[2],
                                DstMacAddr[3],
                                DstMacAddr[4],
                                DstMacAddr[5]));
                            bDstMacSpecified = TRUE;

                            increment = 2;
                            break;
                        }
                    }
                        
                    PRINTF(("Option m needs MAC address parameter\n"));
                    return (FALSE);
                
                case '?':
                    return (FALSE);

                default:
                    PRINTF(("Unknown option %c\n", *pOption));
                    return (FALSE);
            }

            i+= increment;
        }

        pNdisDeviceName = argv[argc-1];
        
    }
    while (FALSE);

    return (bOkay);
}


HANDLE OpenHandle(__in __nullterminated CHAR *pDeviceName)
{
	DWORD   DesiredAccess;
	DWORD   ShareMode;
	LPSECURITY_ATTRIBUTES   lpSecurityAttributes = NULL;

	DWORD   CreationDistribution;
	DWORD   FlagsAndAttributes;
	HANDLE  Handle;
	DWORD   BytesReturned;

	DesiredAccess = GENERIC_READ|GENERIC_WRITE;
	ShareMode = 0;
	CreationDistribution = OPEN_EXISTING;
	FlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;

	Handle = CreateFileA(pDeviceName, DesiredAccess, ShareMode, lpSecurityAttributes, CreationDistribution, FlagsAndAttributes, NULL);
    if (Handle == INVALID_HANDLE_VALUE) {
		DEBUGP(("Creating file failed, error %x\n", GetLastError()));
		return Handle;
	}

	//
	//  Wait for the driver to finish binding.
	//
	if (!DeviceIoControl(Handle, IOCTL_NDISPROT_BIND_WAIT, NULL, 0, NULL, 0, &BytesReturned, NULL)) {
		DEBUGP(("IOCTL_NDISIO_BIND_WAIT failed, error %x\n", GetLastError()));
		CloseHandle(Handle);
		Handle = INVALID_HANDLE_VALUE;
	}

	return (Handle);
}

BOOL OpenNdisDevice(HANDLE Handle,  __in __nullterminated CHAR *pDeviceName)
{
	WCHAR   wNdisDeviceName[MAX_NDIS_DEVICE_NAME_LEN];
	INT     wNameLength;
	INT     NameLength = strlen(pDeviceName);
	DWORD   BytesReturned;
	INT     i;

    //
    // Convert to unicode string - non-localized...
    //
	wNameLength = 0;
	for (i = 0; i < NameLength && i < MAX_NDIS_DEVICE_NAME_LEN-1; i++) {
		wNdisDeviceName[i] = (WCHAR)pDeviceName[i];
		wNameLength++;
	}
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "wNdisDeviceName is bounded by the check above");    
	wNdisDeviceName[i] = L'\0';
	DEBUGP(("Trying to access NDIS Device: %ws\n", wNdisDeviceName));
    return (DeviceIoControl(Handle, IOCTL_NDISPROT_OPEN_DEVICE, (LPVOID)&wNdisDeviceName[0], wNameLength*sizeof(WCHAR), NULL, 0, &BytesReturned, NULL));
}


BOOL GetSrcMac(HANDLE  Handle, PUCHAR  pSrcMacAddr)
{
	DWORD       BytesReturned;
	BOOLEAN     bSuccess;
	UCHAR       QueryBuffer[sizeof(NDISPROT_QUERY_OID) + MAC_ADDR_LEN];
	PNDISPROT_QUERY_OID  pQueryOid;

	DEBUGP(("Trying to get src mac address\n"));

	pQueryOid = (PNDISPROT_QUERY_OID)&QueryBuffer[0];
	pQueryOid->Oid = OID_802_3_CURRENT_ADDRESS;
	pQueryOid->PortNumber = 0;
	bSuccess = (BOOLEAN)DeviceIoControl(Handle, IOCTL_NDISPROT_QUERY_OID_VALUE, (LPVOID)&QueryBuffer[0], sizeof(QueryBuffer), (LPVOID)&QueryBuffer[0], sizeof(QueryBuffer), &BytesReturned, NULL);
    if (bSuccess) {
		DEBUGP(("GetSrcMac: IoControl success, BytesReturned = %d\n", BytesReturned));
#pragma prefast(suppress:__WARNING_WRITE_OVERRUN __WARNING_BUFFER_OVERRUN_NONSTACK __WARNING_READ_OVERRUN __WARNING_BUFFER_OVERRUN, "enough space allocated in QueryBuffer")
		memcpy(pSrcMacAddr, pQueryOid->Data, MAC_ADDR_LEN);                    
	} else {
		DEBUGP(("GetSrcMac: IoControl failed: %d\n", GetLastError()));
	}

	return (bSuccess);
}


USHORT checksum(unsigned short *buffer, int size)
{
	unsigned long cksum = 0;

	while (size > 1) {
		cksum += *buffer++;
		size -= sizeof(USHORT);
	}

	if (size)
		cksum += *(UCHAR*)buffer;

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);

	return (USHORT)(~cksum);
}

void InitIPHeader(PIPV4_HDR Header, char *SourceAddress, const char *DestAddress, unsigned short TotalLength)
{
	Header->ip_verlen = 0x45;
	Header->ip_id = 0;
	Header->ip_offset = 0;
	Header->ip_protocol = 1; // ICMP
	Header->ip_tos = 0;
	Header->ip_checksum = 0;
	Header->ip_totallength = htons(TotalLength);
	Header->ip_ttl = 36;
	Header->ip_srcaddr = inet_addr(SourceAddress);
	Header->ip_destaddr = inet_addr(DestAddress);
//	Header->ip_checksum = htons(checksum((unsigned short *)Header, sizeof(IPV4_HDR)));

	return;
}

void InitIcmpHeader(char *buf, const char *Data, int datasize)
{
	ICMP_HDR   *icmp_hdr = NULL;
	char       *datapart = NULL;

	icmp_hdr = (ICMP_HDR *)buf;
	icmp_hdr->icmp_type = 8;        // request an ICMP echo
	icmp_hdr->icmp_code = 0;
	icmp_hdr->icmp_id = (USHORT)GetCurrentProcessId();
	icmp_hdr->icmp_checksum = 0;
	icmp_hdr->icmp_sequence = 0;
	icmp_hdr->icmp_timestamp = GetTickCount();
	datapart = buf + sizeof(ICMP_HDR);
	memcpy(datapart, Data, datasize);
	icmp_hdr->icmp_checksum = checksum((unsigned short *)buf, datasize + sizeof(ICMP_HDR));

	return;
}



VOID DoWriteProc(HANDLE Handle)
{
	PUCHAR      pWriteBuf = NULL;
	INT         SendCount;
	DWORD       BytesWritten;
	BOOLEAN     bSuccess;

	DEBUGP(("DoWriteProc\n"));
	SendCount = 0;

	do {
		unsigned short dataLength = (unsigned short)strlen(PacketData);
		unsigned short totalLength = sizeof(ETH_HEADER) + sizeof(IPV4_HDR) + sizeof(ICMP_HDR) + dataLength;

		pWriteBuf = malloc(totalLength);
		if (pWriteBuf == NULL)
			break;

		PETH_HEADER pEthHeader = (PETH_HEADER)pWriteBuf;
		PIPV4_HDR ipHeader = (PIPV4_HDR)(pWriteBuf + sizeof(ETH_HEADER));
		PICMP_HDR icmpHeader = (PICMP_HDR)(pWriteBuf + sizeof(ETH_HEADER) + sizeof(IPV4_HDR));

#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW, "pWriteBuf is PacketLength(100 bytes long");        
		pEthHeader->EthType = htons(EthType);
		memcpy(pEthHeader->SrcAddr, SrcMacAddr, MAC_ADDR_LEN);
        
		memcpy(pEthHeader->DstAddr, DstMacAddr, MAC_ADDR_LEN);
		InitIPHeader(ipHeader, SourceIPAddress, DestIPAddress, totalLength - sizeof(ETH_HEADER));
		InitIcmpHeader((char *)icmpHeader, PacketData, dataLength);
		SendCount = 0;
		while (TRUE) {
			bSuccess = (BOOLEAN)WriteFile(Handle, pWriteBuf, totalLength, &BytesWritten, NULL);
			if (!bSuccess) {
				PRINTF(("DoWriteProc: WriteFile failed on Handle %p\n", Handle));
				break;
			}
			
			SendCount++;
			DEBUGP(("DoWriteProc: sent %d bytes\n", BytesWritten));
			if ((NumberOfPackets != -1) && (SendCount == NumberOfPackets))
				break;
		}
	} while (FALSE);

	if (pWriteBuf != NULL)
		free(pWriteBuf);

	return;
}

VOID EnumerateDevices(HANDLE  Handle)
{
	typedef __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) QueryBindingCharBuf;        
	QueryBindingCharBuf		Buf[1024];
	DWORD       		BufLength = sizeof(Buf);
	DWORD       		BytesWritten;
	DWORD       		i;
	PNDISPROT_QUERY_BINDING 	pQueryBinding;

	pQueryBinding = (PNDISPROT_QUERY_BINDING)Buf;

	i = 0;
	for (pQueryBinding->BindingIndex = i; /* NOTHING */; pQueryBinding->BindingIndex = ++i) {
#pragma prefast(suppress: __WARNING_READ_OVERRUN, "Okay to read sizeof(NDISPROT_QUERY_BINDING) from pQueryBinding");    
		if (DeviceIoControl(Handle, IOCTL_NDISPROT_QUERY_BINDING, pQueryBinding, sizeof(NDISPROT_QUERY_BINDING), Buf, BufLength, &BytesWritten, NULL)) {
            PRINTF(("%2d. %ws\n     - %ws\n", pQueryBinding->BindingIndex, (WCHAR *)((PUCHAR)pQueryBinding + pQueryBinding->DeviceNameOffset), (WCHAR *)((PUCHAR )pQueryBinding + pQueryBinding->DeviceDescrOffset)));
			memset(Buf, 0, BufLength);
		} else {
			ULONG   rc = GetLastError();
			if (rc != ERROR_NO_MORE_ITEMS) {
				PRINTF(("EnumerateDevices: terminated abnormally, error %d\n", rc));
			}
            break;
        }
    }
}




int __cdecl main(INT argc, __in_ecount(argc) LPSTR *argv)
{
	HANDLE      DeviceHandle;

	DeviceHandle = INVALID_HANDLE_VALUE;

	do {
		 if (!GetOptions(argc, argv)) {
			PrintUsage();
			break;
		}

		DeviceHandle = OpenHandle(pNdisProtDevice);
		if (DeviceHandle == INVALID_HANDLE_VALUE) {
			PRINTF(("Failed to open %s\n", pNdisProtDevice));
			break;
		}

		if (DoEnumerate) {
			EnumerateDevices(DeviceHandle);
			break;
		}

		if (!OpenNdisDevice(DeviceHandle, pNdisDeviceName)) {
			PRINTF(("Failed to access %s\n", pNdisDeviceName));
			break;
		}

		DEBUGP(("Opened device %s successfully!\n", pNdisDeviceName));
		if (!GetSrcMac(DeviceHandle, SrcMacAddr)) {
			PRINTF(("Failed to obtain local MAC address\n"));
			break;
		}

        DEBUGP(("Got local MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", SrcMacAddr[0], SrcMacAddr[1], SrcMacAddr[2], SrcMacAddr[3], SrcMacAddr[4], SrcMacAddr[5]));

		if (!bDstMacSpecified)
			memcpy(DstMacAddr, SrcMacAddr, MAC_ADDR_LEN);

		DoWriteProc(DeviceHandle);
	} while (FALSE);

	if (DeviceHandle != INVALID_HANDLE_VALUE)
		CloseHandle(DeviceHandle);

	return 0;
}
