#ifndef COM_H
#define COM_H

#include "libkernel.h"

void com1_initialize();
uint8_t com1_read_byte();
void com1_write_byte(uint8_t byte);

#endif // COM_H
