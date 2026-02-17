// io.h
#ifndef IO_H
#define IO_H

#include <stdint.h>

unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char val);
uint32_t inl(unsigned short port);
void outl(unsigned short port, uint32_t val);
void wait_io();
void sys_reboot();

#endif