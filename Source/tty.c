#include <stddef.h>
#include <stdint.h>

#include "vga.h"
#include "tty.h"

typedef struct tty_t {
	k_size_t row;
	k_size_t column;
	uint8_t color;
	ansi_state_t ansi_state;
	int ansi_param;
	uint16_t screen_buff[VGA_WIDTH * VGA_HEIGHT];
} tty_t;

static tty_t g_ttys[MAX_TTYS];
static tty_id_t g_active_tty;

static tty_t *get_tty(tty_id_t id) {
	if (id < 0 || id >= MAX_TTYS) {
		return NULL;
	}

	return &g_ttys[id];
}

void tty_initialize(void) {
	for (int i = 0; i < MAX_TTYS; i += 1) {
		g_ttys[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);;
		tty_clear(i);
	}

	tty_set_active(0);
}

void tty_clear(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	for (int row = 0; row < VGA_HEIGHT; row += 1) {
		for (int col = 0; col < VGA_WIDTH; col += 1) {
			uint16_t entry = vga_entry(0, tty->color);
			tty->screen_buff[row * VGA_WIDTH + col] = entry;

			if (g_active_tty == id) {
				vga_set_entry_at(col, row, entry);
			}
		}
	}

	tty->column = 0;
	tty->row = 0;
}

void tty_scroll_up(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	for (int row = 0; row < VGA_HEIGHT - 1; row += 1) {
		for (int col = 0; col < VGA_WIDTH; col += 1) {
			uint16_t entry = tty->screen_buff[(row + 1) * VGA_WIDTH + col];
			tty->screen_buff[row * VGA_WIDTH + col] = entry;

			if (g_active_tty == id) {
				vga_set_entry_at(col, row, entry);
			}
		}
	}

	for (int col = 0; col < VGA_WIDTH; col += 1) {
		uint16_t entry = vga_entry(0, tty->color);
		tty->screen_buff[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = entry;

		if (g_active_tty == id)  {
			vga_set_entry_at(col, VGA_HEIGHT - 1, entry);
		}
	}

	tty->row -= 1;
}

void tty_set_active(tty_id_t id) {
	if (id < 0 || id >= MAX_TTYS) {
		return;
	}

	tty_t *tty = &g_ttys[id];
	g_active_tty = id;

	for (int row = 0; row < VGA_HEIGHT; row += 1) {
		for (int col = 0; col < VGA_WIDTH; col += 1) {
			uint16_t entry = tty->screen_buff[row * VGA_WIDTH + col];
			vga_set_entry_at(col, row, entry);
		}
	}
}

tty_id_t tty_get_active(void) {
	return g_active_tty;
}

static uint8_t ansi_to_vga(uint8_t ansi) {
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

void tty_putchar(tty_id_t id, char c) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->ansi_state == ANSI_STATE_NORMAL) {
		if (c == '\x1b') {
			tty->ansi_state = ANSI_STATE_ESC;
		} else {
			if (c == '\n' || tty->column == VGA_WIDTH) {
				tty->column = 0;
				tty->row += 1;
			}

			if (tty->row >= VGA_HEIGHT) {
				tty_scroll_up(id);
			}

			if (c == '\n') {
				return;
			}

			uint16_t entry = vga_entry(c, tty->color);

			tty->screen_buff[tty->row * VGA_WIDTH + tty->column] = entry;

			if (g_active_tty == id) {
				vga_set_entry_at(tty->column, tty->row, entry);
			}

			tty->column += 1;
		}
	} else if (tty->ansi_state == ANSI_STATE_ESC) {
		if (c == '[') {
			tty->ansi_state = ANSI_STATE_CSI;
			tty->ansi_param = 0;
		} else {
			tty->ansi_state = ANSI_STATE_NORMAL;
		}
	} else if (tty->ansi_state == ANSI_STATE_CSI) {
		if (c >= '0' && c <= '9') {
			tty->ansi_param = tty->ansi_param * 10 + (c - '0');
		} else {
			// Only handle the 'm' command
			if (c == 'm') {
				if (tty->ansi_param == 0) {
					tty->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
				} else if ((tty->ansi_param >= 30 && tty->ansi_param <= 37) || (tty->ansi_param >= 90 && tty->ansi_param <= 97)) {
					tty->color = vga_entry_color(vga_color_get_fg(tty->color), ansi_to_vga(tty->ansi_param));
				} else if ((tty->ansi_param >= 40 && tty->ansi_param <= 47) || (tty->ansi_param >= 100 && tty->ansi_param <= 107)) {
					tty->color = vga_entry_color(ansi_to_vga(tty->ansi_param - 10), vga_color_get_bg(tty->color));
				}

				tty->ansi_state = ANSI_STATE_NORMAL;
			}
		}
	}
}

void tty_set_color(tty_id_t id, uint8_t color) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	tty->color = color;
}

uint8_t tty_get_color(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return 0;
	}

	return tty->color;
}
