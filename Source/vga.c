#include "vga.h"
#include "ioport.h"

static uint16_t *g_vga_buff = (uint16_t *)0xb8000;

vga_color_t vga_color_get_fg(uint8_t c) {
	return c & 0xf;
}

vga_color_t vga_color_get_bg(uint8_t c) {
	return (c >> 4) & 0xf;
}

uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
	return fg | bg << 4;
}

uint16_t vga_entry(char c, uint8_t color) {
	return (uint16_t)c | (uint16_t) color << 8;
}

uint16_t vga_get_entry_at(int col, int row) {
	if (col < 0 || col >= VGA_WIDTH) {
		return 0;
	}
	if (row < 0 || row >= VGA_HEIGHT) {
		return 0;
	}

	return g_vga_buff[row * VGA_WIDTH + col];
}

void vga_set_entry_at(int col, int row, uint16_t entry) {
	if (col < 0 || col >= VGA_WIDTH) {
		return;
	}
	if (row < 0 || row >= VGA_HEIGHT) {
		return;
	}

	g_vga_buff[row * VGA_WIDTH + col] = entry;
}

enum {
	VGA_PORT_CRT_CONTROLLER_ADDRESS = 0x3d4,
	VGA_PORT_CRT_CONTROLLER_DATA    = 0x3d5,
};

enum {
	VGA_REGISTER_CURSOR_LOCATION_HIGH  = 0x0e,
	VGA_REGISTER_CURSOR_LOCATION_LOW   = 0x0f,
	VGA_REGISTER_CURSOR_SCANLINE_START = 0x0a,
	VGA_REGISTER_CURSOR_SCANLINE_END   = 0x0b,
};

void vga_hide_cursor() {
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_START);
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_DATA, 0x20);
}

void vga_show_cursor() {
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_START);
	uint8_t current = ioport_read_byte(VGA_PORT_CRT_CONTROLLER_DATA);

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_START);
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_DATA, current & 0x1f);
}

void vga_set_cursor_extents(uint8_t start, uint8_t end) {
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_START);
	uint8_t current = ioport_read_byte(VGA_PORT_CRT_CONTROLLER_DATA);

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_START);
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_DATA, current & 0x20 | start);

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_END);
	current = ioport_read_byte(VGA_PORT_CRT_CONTROLLER_DATA);

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_SCANLINE_END);
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_DATA, current & 0x30 | start);
}

int32_t vga_get_cursor_offset() {
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_LOCATION_HIGH);
	int32_t offset = ioport_read_byte(VGA_PORT_CRT_CONTROLLER_DATA);
	offset <<= 8;

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_LOCATION_LOW);
	offset |= ioport_read_byte(VGA_PORT_CRT_CONTROLLER_DATA);

	return offset;
}

void vga_get_cursor_position(int *col, int *row) {
	int32_t offset = vga_get_cursor_offset();
	if (col) {
		*col = offset % VGA_WIDTH;
	}
	if (row) {
		*row = offset / VGA_WIDTH;
	}
}

void vga_set_cursor_offset(int32_t offset) {
	if (offset < 0) {
		offset = 0;
	}
	if (offset >= VGA_WIDTH * VGA_HEIGHT) {
		offset = VGA_WIDTH * VGA_HEIGHT - 1;
	}

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_LOCATION_LOW);
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_DATA, (uint8_t)(offset & 0xff));

	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_ADDRESS, VGA_REGISTER_CURSOR_LOCATION_HIGH);
	ioport_write_byte(VGA_PORT_CRT_CONTROLLER_DATA, (uint8_t)((offset >> 8) & 0xff));
}

void vga_set_cursor_position(int col, int row) {
	if (col < 0) {
		col = 0;
	}
	if (col >= VGA_WIDTH) {
		col = VGA_WIDTH - 1;
	}
	if (row < 0) {
		row = 0;
	}
	if (row >= VGA_HEIGHT) {
		row = VGA_HEIGHT - 1;
	}

	vga_set_cursor_offset(row * VGA_WIDTH + col);
}
