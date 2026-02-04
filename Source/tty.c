#include <stddef.h>
#include <stdint.h>

#include "vga.h"
#include "tty.h"
#include "libkernel.h"

static size_t g_terminal_row;
static size_t g_terminal_column;
uint8_t g_terminal_color;
uint16_t* g_terminal_buffer = (uint16_t*)VGA_MEMORY;

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

void terminal_initialize(void)
{
    g_terminal_row = 0;
    g_terminal_column = 0;
    g_terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            g_terminal_buffer[index] = vga_entry(0, g_terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color)
{
    g_terminal_color = color;
}

void terminal_clear(void)
{
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        g_terminal_buffer[i] = vga_entry(0, g_terminal_color);
    }
    g_terminal_row = 0;
    g_terminal_column = 0;
}

void terminal_scroll_down(void)
{
    for(size_t i = 0; i < (VGA_WIDTH * (VGA_HEIGHT - 1)); i++) {
        g_terminal_buffer[i] = g_terminal_buffer[i + VGA_WIDTH];
    }
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    g_terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        if (++g_terminal_row == VGA_HEIGHT) {
            terminal_scroll_down();
            g_terminal_row = VGA_HEIGHT - 1;
        }
        g_terminal_column = 0;

        return ;
    }

    terminal_putentryat(c, g_terminal_color, g_terminal_column, g_terminal_row);

    if (++g_terminal_column == VGA_WIDTH) {
        g_terminal_column = 0;

        if (++g_terminal_row == VGA_HEIGHT) {
            g_terminal_row = 0;
        }
    }
}

void terminal_putstr(const char* str)
{
    char		c;
    ansi_state_t	ansi_state = ANSI_STATE_NORMAL;
    int			param;

    while (*str) {
        c = *str++;

        if ( ansi_state == ANSI_STATE_NORMAL) {
		if (c == '\x1b')
			ansi_state = ANSI_STATE_ESC;
		else
			terminal_putchar(c);
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
					g_terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
				}
				else if ((param >= 30 && param <= 37) || (param >= 90 && param <= 97)) {
					g_terminal_color = vga_entry_color_fg(ansi_to_vga(param));
				}
				else if ((param >= 40 && param <= 47) || (param >= 100 && param <= 107)) {
					g_terminal_color = vga_entry_color_bg(ansi_to_vga(param - 10));
				}
				ansi_state = ANSI_STATE_NORMAL;
			}
		}
	}
    }
}