#include "net.h"
#include "screen.h"

static struct net_device *default_net_dev = 0;

void net_register_device(struct net_device *dev) {
    if (!dev) return;
    
    print_string("NET: Registering device ");
    print_string(dev->name);
    print_string("\n");
    
    if (!default_net_dev) {
        default_net_dev = dev;
        print_string("NET: Set as default network device.\n");
    }
}
