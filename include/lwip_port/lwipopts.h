#ifndef LWIP_LWIPOPTS_H
#define LWIP_LWIPOPTS_H

// Operating System configuration
#define NO_SYS 1           // Bare metal, no RTOS
#define SYS_LIGHTWEIGHT_PROT 0

// Memory allocation
#define MEM_LIBC_MALLOC 0  // Use lwIP's internal memory manager
#define MEM_ALIGNMENT 4
#define MEM_SIZE 128000    // 128 KB for lwIP heap
#define MEMP_NUM_PBUF 16
#define MEMP_NUM_TCP_PCB 8
#define PBUF_POOL_SIZE 16

// Protocol enabling
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ARP 1
#define LWIP_ICMP 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_DHCP 0 // Disable DHCP to simplify manual static IP
#define LWIP_DNS 0
#define LWIP_NETCONN 0 // Requires RTOS
#define LWIP_SOCKET 0  // Requires RTOS

// Checksum by software
#define CHECKSUM_GEN_IP 1
#define CHECKSUM_GEN_UDP 1
#define CHECKSUM_GEN_TCP 1
#define CHECKSUM_GEN_ICMP 1
#define CHECKSUM_CHECK_IP 1
#define CHECKSUM_CHECK_UDP 1
#define CHECKSUM_CHECK_TCP 1
#define CHECKSUM_CHECK_ICMP 1

#endif
