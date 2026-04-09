#include "rtl8169.h"
#include "pci.h"
#include "io.h"
#include "screen.h"
#include "kmalloc.h"
#include "net.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/dhcp.h"

// Track the memory mapped I/O base
static uint8_t *mmio_base = 0;

// Descriptor Rings (1K bytes each supports roughly 64 descriptors)
#define NUM_DESCS 64
static rtl_desc_t *tx_ring;
static rtl_desc_t *rx_ring;

// Receive Buffer Backing Memory
static uint8_t rx_buffers[NUM_DESCS][1536];

// Direct MMIO Reading and Writing
static void rtl_outb(uint8_t reg, uint8_t val) {
    if (mmio_base) *(volatile uint8_t *)(mmio_base + reg) = val;
}

static uint8_t rtl_inb(uint8_t reg) {
    return mmio_base ? *(volatile uint8_t *)(mmio_base + reg) : 0;
}

static void rtl_outd(uint8_t reg, uint32_t val) {
    if (mmio_base) *(volatile uint32_t *)(mmio_base + reg) = val;
}

static struct netif rtl_netif;
static int tx_idx = 0;

uint32_t rtl8169_tx_packets = 0;
uint32_t rtl8169_rx_packets = 0;
uint32_t rtl8169_tx_bytes = 0;
uint32_t rtl8169_rx_bytes = 0;

err_t rtl8169_linkoutput(struct netif *netif, struct pbuf *p) {
    (void)netif;
    if (!tx_ring) return ERR_IF;
    
    // Copy lwIP packet buffer into flat memory for hardware DMA
    static uint8_t flat_buf[1536]; 
    pbuf_copy_partial(p, flat_buf, p->tot_len, 0);
    
    // Assign to physical transmit ring location
    tx_ring[tx_idx].buf_low = (uint32_t)(uint64_t)flat_buf;
    tx_ring[tx_idx].buf_high = 0;
    
    // Setting OWN bit (Hardware assumes control), First Segment, Last Segment, and Length
    uint32_t cmd = 0x80000000 | 0x20000000 | 0x10000000 | (p->tot_len & 0xFFFF);
    if (tx_idx == NUM_DESCS - 1) cmd |= 0x40000000; // End of Ring wrap marker
    
    tx_ring[tx_idx].command = cmd;
    
    // Write 0x40 to TPPOLL to tell hardware memory is ready
    rtl_outb(RTL_TPPOLL, 0x40);
    
    rtl8169_tx_packets++;
    rtl8169_tx_bytes += p->tot_len;
    
    tx_idx = (tx_idx + 1) % NUM_DESCS;
    return ERR_OK;
}

static int rx_idx = 0;
void rtl8169_poll(void) {
    if (!rx_ring) return;
    
    // Process all available packets in the ring
    while ((rx_ring[rx_idx].command & RTL_DESC_OWN) == 0) {
        uint32_t cmd = rx_ring[rx_idx].command;
        int len = cmd & 0x3FFF; // Extract packet length from lower 14 bits
        
        if (len > 0 && len <= 1536) {
            rtl8169_rx_packets++;
            rtl8169_rx_bytes += len;
            
            // Allocate a packet buffer for lwIP
            struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
            if (p != NULL) {
                pbuf_take(p, rx_buffers[rx_idx], len);
                
                // Inject the packet directly into the lwIP network stack!
                if (rtl_netif.input(p, &rtl_netif) != ERR_OK) {
                    pbuf_free(p);
                }
            }
        }
        
        // Reset the descriptor and give ownership back to the Hardware
        uint32_t reset_cmd = RTL_DESC_OWN | 1536;
        if (rx_idx == NUM_DESCS - 1) reset_cmd |= 0x40000000; // Ring wrap marker
        rx_ring[rx_idx].command = reset_cmd;
        
        rx_idx = (rx_idx + 1) % NUM_DESCS;
    }
}

err_t rtl8169_netif_init(struct netif *netif) {
    netif->name[0] = 'r';
    netif->name[1] = 't';
    netif->linkoutput = rtl8169_linkoutput;     // Bind local transmission to lwIP
    netif->output = etharp_output;              // Use standard ARP resolver for IPv4
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    
    // Pull the real hardware MAC Address directly from config registers
    for (int i=0; i<6; i++) {
        netif->hwaddr[i] = rtl_inb(RTL_MAC0 + i);
    }
    netif->hwaddr_len = 6;
    return ERR_OK;
}

void rtl8169_init(uint32_t bus, uint32_t device, uint32_t function) {
    static int is_initialized = 0;
    if (is_initialized) {
        print_string("RTL8169: Already initialized. Skipping to avoid lwIP crashes.\n");
        return;
    }
    is_initialized = 1;

    print_string("RTL8169: Initializing hardware...\n");
    
    // Read BAR2 (Offset 0x18) or BAR0 (Offset 0x10) to find MMIO space.
    // Usually BAR2 or BAR3 is MMIO for Realtek NICs. Let's look exactly at BAR2 first.
    uint32_t bar2 = pci_read_config_dword(bus, device, function, 0x18);
    // Mask off the type flag bits to get the raw physical address
    mmio_base = (uint8_t *)(uint64_t)(bar2 & 0xFFFFFFF0);
    
    if (!mmio_base) {
        print_string("RTL8169: Failed to find valid MMIO space!\n");
        return;
    }

    // Step 1: Unlock Configuration Registers
    print_string("RTL8169: Unlocking Config...\n");
    rtl_outb(RTL_9346CR, RTL_9346CR_UNLOCK);

    // Step 2: Issue Software Reset
    print_string("RTL8169: Performing hardware reset...\n");
    rtl_outb(RTL_COMMAND, RTL_CMD_RESET);
    
    uint32_t timeout = 100000;
    while ((rtl_inb(RTL_COMMAND) & RTL_CMD_RESET) && timeout > 0) {
        timeout--;
    }
    if (timeout == 0) {
        print_string("RTL8169: WARNING - HW Reset timeout.\n");
    }

    // Lock config back
    rtl_outb(RTL_9346CR, RTL_9346CR_LOCK);

    // Step 3: Setup Rings
    // We allocate 1024 bytes (1 KB) for the 64-descriptor arrays.
    // They must be page-aligned for DMA. The physical address is required.
    // Since we use identity paging in physical memory, the kmalloc pointer is the physical ptr.
    // (Assuming kmalloc returns page/256-byte aligned. For safety we just use it directly in our flat map).
    tx_ring = (rtl_desc_t *)kmalloc(1024);
    rx_ring = (rtl_desc_t *)kmalloc(1024);

    if (tx_ring && rx_ring) {
        print_string("RTL8169: DMA Rings allocated.\n");
        
        // Setup Rx Ring Buffers
        for (int i = 0; i < NUM_DESCS; i++) {
            rx_ring[i].vlan = 0;
            rx_ring[i].buf_high = 0;
            rx_ring[i].buf_low = (uint32_t)(uint64_t)rx_buffers[i];
            
            // OWN bit = 1 (Hardware owns it), Max size = 1536
            uint32_t cmd = RTL_DESC_OWN | 1536;
            if (i == NUM_DESCS - 1) cmd |= 0x40000000; // EOR marker
            rx_ring[i].command = cmd;
        }

        // Tell hardware where the descriptor rings are
        rtl_outd(RTL_TX_DESC_START, (uint32_t)(uint64_t)tx_ring);
        // RTL8169 uses high 32 bits register directly after for 64-bit DMA, assuming 0 for now.
        rtl_outd(RTL_TX_DESC_START + 4, 0);

        rtl_outd(RTL_RX_DESC_START, (uint32_t)(uint64_t)rx_ring);
        rtl_outd(RTL_RX_DESC_START + 4, 0);
    }

    // Step 4: Enable Receiver (RE) and Transmitter (TE)
    print_string("RTL8169: Configuring RX Filters & Enabling RX/TX logic...\n");
    // RCR: Accept Physical Match (0x02) | Accept Multicast (0x04) | Accept Broadcast (0x08)
    rtl_outd(RTL_RCR, 0x0E);
    rtl_outb(RTL_COMMAND, RTL_CMD_RE | RTL_CMD_TE);

    print_string("RTL8169: Hardware Initialization Complete. Bootstrapping lwIP...\n");
    
    // Step 5: Mount hardware into software network stack!
    lwip_init();
    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 0, 0, 0, 0);
    IP4_ADDR(&netmask, 0, 0, 0, 0);
    IP4_ADDR(&gw, 0, 0, 0, 0);
    
    netif_add(&rtl_netif, &ipaddr, &netmask, &gw, NULL, rtl8169_netif_init, netif_input);
    netif_set_default(&rtl_netif);
    netif_set_up(&rtl_netif);

    dhcp_start(&rtl_netif);
    
    print_string("RTL8169: lwIP Bound to Hardware! DHCP Discovery in progress...\n");
}
