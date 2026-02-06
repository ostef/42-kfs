#include "vga.h"

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
