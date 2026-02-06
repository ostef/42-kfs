#include "com.h"
#include "ioport.h"

#define COM1_PORT 0x3f8

static bool g_com1_initialized;

void com1_initialize() {
	ioport_write_byte(COM1_PORT + 1, 0x00);    // Disable all interrupts
	ioport_write_byte(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
	ioport_write_byte(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
	ioport_write_byte(COM1_PORT + 1, 0x00);    //                  (hi byte)
	ioport_write_byte(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
	ioport_write_byte(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
	ioport_write_byte(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
	ioport_write_byte(COM1_PORT + 4, 0x1E);    // Set in loopback mode, test the serial chip

	ioport_write_byte(COM1_PORT + 0, 0xAE);    // Test serial chip (send byte 0xAE and check if serial returns same byte)

	// Check if serial is faulty (i.e: not same byte as sent)
	bool faulty = ioport_read_byte(COM1_PORT) != 0xae;
	if (faulty) {
		k_printf("Could not initialize COM1 serial port for debugging\n");
		return;
	}

	// If serial is not faulty set it in normal operation mode
	// (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
	ioport_write_byte(COM1_PORT + 4, 0x0F);

	g_com1_initialized = true;

	k_printf("Intialized COM1 serial port for debugging\n");
}

#define MAX_COM1_WAIT 10

uint8_t com1_read_byte() {
	if (!g_com1_initialized) {
		return 0;
	}

	// Wait until we can read
	int i = 0;
	while (i < MAX_COM1_WAIT && (ioport_read_byte(COM1_PORT + 5) & 0x01) == 0) {
		i += 1;
	}

	if (i == MAX_COM1_WAIT) {
		return 0;
	}

	return ioport_read_byte(COM1_PORT);
}

void com1_write_byte(uint8_t byte) {
	if (!g_com1_initialized) {
		return;
	}

	// Wait until transit is empty
	int i = 0;
	while ((ioport_read_byte(COM1_PORT + 5) & 0x20) == 0) {
		i += 1;
	}

	if (i == MAX_COM1_WAIT) {
		return;
	}

	ioport_write_byte(COM1_PORT, byte);
}
