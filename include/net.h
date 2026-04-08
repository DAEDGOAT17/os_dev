#ifndef NET_H
#define NET_H

#include <stdint.h>

#define MAC_ADDR_LEN 6

struct net_device {
    char name[16];
    uint8_t mac_addr[MAC_ADDR_LEN];
    uint32_t ip_addr; // simple ipv4 for now
    
    // Status flags
    uint8_t up;
    uint8_t link_state;
    
    // Function pointers for driver operations
    int (*init)(struct net_device *dev);
    int (*send)(struct net_device *dev, const void *data, uint32_t len);
    
    // Callback when data is received
    void (*rx_callback)(struct net_device *dev, const void *data, uint32_t len);
    
    // Driver specific private data (e.g. MMIO base, firmware structs)
    void *priv;
};

// Register a network device
void net_register_device(struct net_device *dev);

#endif
