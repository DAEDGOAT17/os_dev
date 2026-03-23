// io.h
#ifndef IO_H
#define IO_H

#include <stdint.h>

unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char val);
unsigned short inw(unsigned short port);
void outw(unsigned short port, unsigned short val);
void sys_reboot();

#endif