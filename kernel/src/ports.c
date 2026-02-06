#include <ports.h>
#include <types/types.h>

inline void write_port_u8(uint16_t port, uint8_t val) {
	asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : );
}

inline uint8_t read_port_u8(uint16_t port) {
	uint8_t ret;
	asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : );
	return ret;
}

inline void insw(uint16_t port, void *buf, unsigned long n) {
	asm volatile("cld; rep; insw" : "+D"(buf), "+c"(n) : "d"(port));
}

inline void outsw(uint16_t port, const void *buf, unsigned long n) {
	asm volatile("cld; rep; outsw" : "+S"(buf), "+c"(n) : "d"(port));
}
