#include <stddef.h>
#include <stdint.h>

#include "vga.h"
#include "tty.h"

static size_t g_tty_row;
static size_t g_tty_column;
uint8_t g_tty_color;
uint16_t* g_tty_buffer = (uint16_t*)VGA_MEMORY;

uint8_t ansi_to_vga(uint8_t ansi) {
	switch (ansi) {
		case 30: return VGA_COLOR_BLACK;
		case 31: return VGA_COLOR_RED;
		case 32: return VGA_COLOR_GREEN;
		case 33: return VGA_COLOR_BROWN;
		case 34: return VGA_COLOR_BLUE;
		case 35: return VGA_COLOR_MAGENTA;
		case 36: return VGA_COLOR_CYAN;
		case 37: return VGA_COLOR_LIGHT_GREY;
		case 90: return VGA_COLOR_DARK_GREY;
		case 91: return VGA_COLOR_LIGHT_RED;
		case 92: return VGA_COLOR_LIGHT_GREEN;
		case 93: return VGA_COLOR_LIGHT_BROWN;
		case 94: return VGA_COLOR_LIGHT_BLUE;
		case 95: return VGA_COLOR_LIGHT_MAGENTA;
		case 96: return VGA_COLOR_LIGHT_CYAN;
		case 97: return VGA_COLOR_WHITE;
		default: return VGA_COLOR_LIGHT_GREY;
	}
}

void tty_initialize(void)
{
	g_tty_row = 0;
	g_tty_column = 0;
	g_tty_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			g_tty_buffer[index] = vga_entry(0, g_tty_color);
		}
	}
}

void tty_setcolor(uint8_t color)
{
	g_tty_color = color;
}

void tty_clear(void)
{
	for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
		g_tty_buffer[i] = vga_entry(0, g_tty_color);
	}
	g_tty_row = 0;
	g_tty_column = 0;
}

void tty_scroll_down(void)
{
	for(size_t i = 0; i < (VGA_WIDTH * (VGA_HEIGHT - 1)); i++) {
		g_tty_buffer[i] = g_tty_buffer[i + VGA_WIDTH];
	}
}

void tty_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	g_tty_buffer[index] = vga_entry(c, color);
}

void tty_putchar(char c) {
	if (c == '\n') {
		if (++g_tty_row == VGA_HEIGHT) {
			tty_scroll_down();
			g_tty_row = VGA_HEIGHT - 1;
		}
		g_tty_column = 0;

		return ;
	}

	tty_putentryat(c, g_tty_color, g_tty_column, g_tty_row);

	if (++g_tty_column == VGA_WIDTH) {
		g_tty_column = 0;

		if (++g_tty_row == VGA_HEIGHT) {
			g_tty_row = 0;
		}
	}
}

k_size_t tty_putstr(const char* str)
{
	k_size_t			i;
	char			c;
	ansi_state_t	ansi_state = ANSI_STATE_NORMAL;
	int				param;

	i = 0;
	while (str[i]) {
		c = str[i++];

		if ( ansi_state == ANSI_STATE_NORMAL) {
		if (c == '\x1b')
			ansi_state = ANSI_STATE_ESC;
		else
			tty_putchar(c);
		}
	else if (ansi_state == ANSI_STATE_ESC) {
		if (c == '[') {
			ansi_state = ANSI_STATE_CSI;
			param = 0;
		}
		else
			ansi_state = ANSI_STATE_NORMAL;
	}
	else if (ansi_state == ANSI_STATE_CSI) {
		if (c >= '0' && c <= '9') {
			param = param * 10 + (c - '0');
		}
		else {
			// Only handle the 'm' command
			if (c == 'm') {
				if (param == 0) {
					g_tty_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
				}
				else if ((param >= 30 && param <= 37) || (param >= 90 && param <= 97)) {
					g_tty_color = vga_entry_color_fg(ansi_to_vga(param));
				}
				else if ((param >= 40 && param <= 47) || (param >= 100 && param <= 107)) {
					g_tty_color = vga_entry_color_bg(ansi_to_vga(param - 10));
				}
				ansi_state = ANSI_STATE_NORMAL;
			}
		}
	}
	}
	return i;
}