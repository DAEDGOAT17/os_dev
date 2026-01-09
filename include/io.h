// io.h
#ifndef IO_H
#define IO_H

#include <stdint.h>
unsigned char inb(unsigned short port);
void outb(unsigned short port, unsigned char val);
void pic_remap(void);
void pic_unmask_irq(uint8_t irq);


#endif