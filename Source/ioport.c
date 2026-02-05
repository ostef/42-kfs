#include "ioport.h"

uint8_t ioport_read_byte(uint16_t port) {
	uint8_t result;
	asm volatile("in %%dx, %%al" : "=a" (result) : "d" (port));

	return result;
}

uint16_t ioport_read_word(uint16_t port) {
	uint8_t result;
	asm volatile("in %%dx, %%ax" : "=a" (result) : "d" (port));

	return result;
}

void ioport_write_byte(uint16_t port, uint8_t value) {
	asm volatile("out %%al, %%dx" : : "a" (value), "d" (port));
}

void ioport_write_word(uint16_t port, uint16_t value) {
	asm volatile("out %%ax, %%dx" : : "a" (value), "d" (port));
}
