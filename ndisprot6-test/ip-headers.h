
#ifndef __IP_HEADERS_H__
#define __IP_HEADERS_H__


#include <windows.h>



#define MAC_ADDR_LEN                    6


#include <pshpack1.h>


// Ethernet header

typedef struct _ETH_HEADER {
	unsigned char DstAddr[MAC_ADDR_LEN];
	unsigned char SrcAddr[MAC_ADDR_LEN];
	unsigned short EthType;
} ETH_HEADER, *PETH_HEADER;

// IPv4 header

typedef struct ip_hdr {
	unsigned char  ip_verlen;        // 4-bit IPv4 version, 4-bit header length (in 32-bit words)
	unsigned char  ip_tos;           // IP type of service
	unsigned short ip_totallength;   // Total length
	unsigned short ip_id;            // Unique identifier
	unsigned short ip_offset;        // Fragment offset field
	unsigned char  ip_ttl;           // Time to live
	unsigned char  ip_protocol;      // Protocol(TCP,UDP etc)
	unsigned short ip_checksum;      // IP checksum
	unsigned int   ip_srcaddr;       // Source address
	unsigned int   ip_destaddr;      // Source address
} IPV4_HDR, *PIPV4_HDR, FAR * LPIPV4_HDR;

// ICMP header

typedef struct icmp_hdr {
	unsigned char   icmp_type;
	unsigned char   icmp_code;
	unsigned short  icmp_checksum;
	unsigned short  icmp_id;
	unsigned short  icmp_sequence;
	unsigned long   icmp_timestamp;
} ICMP_HDR, *PICMP_HDR, FAR *LPICMP_HDR;

#include <poppack.h>

typedef struct addrinfo {
	int             ai_flags;
	int             ai_family;
	int             ai_socktype;
	int             ai_protocol;
	size_t          ai_addrlen;
	char            *ai_canonname;
	struct sockaddr  *ai_addr;
	struct addrinfo  *ai_next;
} ADDRINFOA, *PADDRINFOA;




#endif 
