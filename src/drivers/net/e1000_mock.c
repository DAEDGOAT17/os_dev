#include "net.h"
#include "screen.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"

static struct netif qemu_netif;

// Mocks the hardware Transmit Ring
static err_t dummy_linkoutput(struct netif *netif, struct pbuf *p) {
    (void)netif;
    set_text_color(MAKE_COLOR(COLOR_LIGHT_MAGENTA, COLOR_BLACK));
    print_string("\n[QEMU E1000 Mock] Successfully intercepted Outgoing Packet: ");
    kprint_dec(p->tot_len);
    print_string(" bytes.\nJARVIS [/] $ ");
    reset_text_color();
    return ERR_OK;
}

static err_t dummy_netif_init(struct netif *netif) {
    netif->name[0] = 'e';
    netif->name[1] = '0';
    netif->linkoutput = dummy_linkoutput;     
    netif->output = etharp_output;              
    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    netif->hwaddr_len = 6;
    // Assign a fake MAC Address
    for (int i=0; i<6; ++i) netif->hwaddr[i] = 0x52 + i;
    return ERR_OK;
}

void qemu_net_init() {
    print_string("QEMU E1000: Found Virtual NIC. Initializing lwIP Mock Bridge...\n");
    lwip_init();
    ip4_addr_t ipaddr, netmask, gw;
    IP4_ADDR(&ipaddr, 192, 168, 1, 100);    
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 1, 1);
    
    netif_add(&qemu_netif, &ipaddr, &netmask, &gw, NULL, dummy_netif_init, netif_input);
    netif_set_default(&qemu_netif);
    netif_set_up(&qemu_netif);
    
    print_string("QEMU E1000: lwIP Bound to Mock Hardware! Subnet: 192.168.1.100\n");
}
