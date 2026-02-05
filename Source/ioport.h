#ifndef IOPORT_H
#define IOPORT_H

#include "libkernel.h"

uint8_t ioport_read_byte(uint16_t port);
uint16_t ioport_read_word(uint16_t port);
void ioport_write_byte(uint16_t port, uint8_t value);
void ioport_write_word(uint16_t port, uint16_t value);

#endif // IOPORT_H
