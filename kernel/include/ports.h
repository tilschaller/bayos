#ifndef _PORTS_H
#define _PORTS_H

#include <types/types.h>

void write_port_u8(uint16_t port, uint8_t val);
uint8_t read_port_u8(uint16_t port);

void insw(uint16_t port, void *buf, unsigned long n);
void outsw(uint16_t port, const void *buf, unsigned long n);

#endif // _PORTS_H
