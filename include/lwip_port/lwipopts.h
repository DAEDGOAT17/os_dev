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
#define PBUF_POOL_SIZE 32
#define PBUF_POOL_BUFSIZE 1536

// TCP buffer sizes — CRITICAL for large HTTP POST requests.
// Default TCP_SND_BUF = 2*TCP_MSS = 1072 bytes, which is too small
// for our Ollama POST (system prompt makes it 3000+ bytes).
// tcp_write() silently returns ERR_MEM if the payload exceeds this!
#define TCP_MSS          1460          // Standard Ethernet MSS
#define TCP_SND_BUF      (8 * 1024)   // 8 KB send buffer (fits full POST)
#define TCP_SND_QUEUELEN (4 * TCP_SND_BUF / TCP_MSS)  // Enough queue slots
#define MEMP_NUM_TCP_SEG TCP_SND_QUEUELEN  // Must be >= TCP_SND_QUEUELEN (lwIP sanity check)
#define TCP_WND          (4 * 1024)   // 4 KB receive window

// Protocol enabling
#define LWIP_IPV4 1
#define LWIP_IPV6 0
#define LWIP_ARP 1
#define LWIP_ICMP 1
#define LWIP_UDP 1
#define LWIP_TCP 1
#define LWIP_RAW 1
#define LWIP_DHCP 1 // Enable DHCP for dynamic IP assignment
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
