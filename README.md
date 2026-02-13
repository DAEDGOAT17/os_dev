# Jarvis OS - Educational OS Kernel

> A custom 32-bit x86 monolithic kernel built from scratch for learning OS internals.

**Status**: Early Development | **License**: Open Source | **Target**: Students & Learning

---

## 🎯 Quick Start

```bash
chmod +x run.sh
./run.sh
```

## Architecture

**Memory Layout**: Kernel mapped to the higher half (0xC0000000) with lower memory reserved for user space.

**Interrupt Pipeline**: Hardware IRQs → PIC routing → IDT lookup → assembly stub → C handler → driver response.

## Roadmap

- [x] Global Descriptor Table
- [x] Interrupt Descriptor Table & PIC
- [x] Physical Memory Manager
- [x] Virtual Memory Manager
- [x] Keyboard and VGA drivers
- [x] Shell
- [ ] Heap allocator
- [ ] Multitasking
- [ ] System calls

---

## License

Open-source for educational use.
