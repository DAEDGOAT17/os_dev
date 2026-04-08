# Jarvis OS: Open Source Acknowledgements

Jarvis OS is a custom 64-bit bare-metal operating system built entirely from scratch. However, to achieve complex routing and high-performance neural network inference without reinventing the wheel, we leverage several ultra-lightweight open-source projects designed specifically for embedded/bare-metal environments.

### 1. lwIP (Lightweight IP Stack)
* **Version:** 2.1.3 (Stable)
* **Path:** `src/net/lwip/`
* **License:** Modified BSD License
* **Description:** A highly respected implementation of the TCP/IP stack specifically designed for embedded systems that lack an RTOS (Real-Time Operating System). 
* **Our Usage:** We use the core `lwIP` state machines (compiled strictly with `NO_SYS=1` and overriding `ctype.h`) to handle IPv4, ARP, and TCP sliding windows instead of manually crafting packet sequence ACKs. The network hooks directly into our `pci.c` detected Realtek RTL8169 DMA rings.

### 3. FAT32 Reference Implementations
* **Description:** The structural definitions (`bpb_t`, Root Directory layouts) utilized in `fat32.c` and `gpt.c` are derived from the foundational EFI configuration specs originally established by Microsoft.

### 4. QEMU simulator & GRUB
* **License:** GNU GPL
* **Description:** While not strictly inside our kernel, our boot logic extensively utilizes GRUB's Multiboot2 specification, and all hardware emulation (like the Intel E1000 and Intel IDE controllers you just viewed) is powered by the incredible QEMU project during our testing cycle!

---

## Appendix: Maintaining a Lightweight lwIP Build
To ensure the OS compiles instantly and doesn't suffer from code bloat, we strictly pruned the lwIP source tree down to its absolute bare necessities. 

**If you ever update the lwIP source tree or fetch a newer release, you MUST delete the following files manually:**
* `src/netif/ppp/` (Serial PPP Dial-up)
* `src/netif/slipif.c` (Serial Line IP)
* `src/netif/lowpan*` (6LoWPAN Bluetooth/Zigbee protocols)
* `src/netif/bridgeif*` (Virtual MAC bridging)
* `src/core/ipv6/` (IPv6 Support is disabled via `LWIP_IPV6 0`)
* `src/apps/` (Unused HTTP/MQTT daemons)
* `src/api/` (RTOS-dependent socket layers)

**CRITICAL RULE:** Never delete `src/netif/ethernet.c`. This is the core parser that slices hardware MAC frames to distribute ARP and IPv4 packets!
