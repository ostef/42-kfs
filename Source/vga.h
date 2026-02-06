#ifndef VGA_H
#define VGA_H

#include "libkernel.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

typedef uint8_t vga_color_t;
enum {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_PINK = 13,
	VGA_COLOR_YELLOW = 14,
	VGA_COLOR_WHITE = 15,
};

vga_color_t vga_color_get_fg(uint8_t c);
vga_color_t vga_color_get_bg(uint8_t c);
uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg);
uint16_t vga_entry(char c, uint8_t color);
uint16_t vga_get_entry_at(int col, int row);
void vga_set_entry_at(int col, int row, uint16_t entry);

void vga_hide_cursor();
void vga_show_cursor();
void vga_set_cursor_extents(uint8_t start, uint8_t end);
int32_t vga_get_cursor_offset();
void vga_get_cursor_position(int *col, int *row);
void vga_set_cursor_offset(int32_t offset);
void vga_set_cursor_position(int col, int row);

#endif // VGA_H