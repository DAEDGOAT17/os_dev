# Jarvis OS - Modern 64-bit Educational Kernel
> A powerful x86_64 monolithic kernel built from scratch for learning OS internals and AI integration.

**Status**: Active Development | **License**: Open Source | **Target**: Modern Laptops (x86_64)

---

## 🎯 Quick Start (Build & Run)
```bash
./run.sh --run        # Runs in BIOS (Legacy) mode
./run.sh --run --uefi # Runs in UEFI (Modern) mode
```

## Architecture
- **Mode**: 64-bit Long Mode (x86_64)
- **Boot**: Hybrid ISO supporting both BIOS and UEFI (Multiboot 2)
- **Memory**: Kernel mapped to the higher half (8GB boundary) with 4GB identity mapping fallback.
- **Interrupts**: Advanced APIC / IO-APIC logic for modern CPU interrupt routing (PICS disabled).

## Roadmap
- [x] 64-bit Long Mode Transition
- [x] UEFI Boot Support (Multiboot 2)
- [x] Global Descriptor Table (64-bit)
- [x] IDT & APIC / IO-APIC Routing
- [x] Physical Memory Manager (Bitmap)
- [x] Virtual Memory Manager (4-level paging)
- [x] Hybrid BIOS/UEFI Framebuffer Support
- [x] Keyboard and VGA (Virtio-VGA) drivers
- [x] Shell with **NLP Intent Mapping**
- [x] Kernel Heap Allocator
- [x] Round-Robin Scheduler
- [x] ATA PIO & FAT32 Filesystem Support
- [x] AI Brain (LLM Engine) Integration
- [ ] User Mode (Ring 3)
- [ ] System Call Interface
- [ ] SMP (Symmetric Multi-Processing)
