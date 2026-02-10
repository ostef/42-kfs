#include "tty.h"
#include "vga.h"

typedef uint8_t ansi_state_t;
enum {
	ANSI_STATE_NORMAL,
	ANSI_STATE_ESC,
	ANSI_STATE_CSI
};

typedef struct tty_t {
	k_size_t row;
	k_size_t column;
	bool cursor_visible;
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
		g_ttys[i].color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
		g_ttys[i].cursor_visible = true;
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

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
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

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_set_active(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	g_active_tty = id;

	for (int row = 0; row < VGA_HEIGHT; row += 1) {
		for (int col = 0; col < VGA_WIDTH; col += 1) {
			uint16_t entry = tty->screen_buff[row * VGA_WIDTH + col];
			vga_set_entry_at(col, row, entry);
		}
	}

	if (tty->cursor_visible) {
		vga_show_cursor();
	} else {
		vga_hide_cursor();
	}

	vga_set_cursor_position(tty->column, tty->row);
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
		case 93: return VGA_COLOR_YELLOW;
		case 94: return VGA_COLOR_LIGHT_BLUE;
		case 95: return VGA_COLOR_PINK;
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

	switch (tty->ansi_state) {
	case ANSI_STATE_NORMAL: {
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
				if (g_active_tty == id) {
					vga_set_cursor_position(tty->column, tty->row);
				}

				return;
			}

			uint16_t entry = vga_entry(c, tty->color);

			tty->screen_buff[tty->row * VGA_WIDTH + tty->column] = entry;

			if (g_active_tty == id) {
				vga_set_entry_at(tty->column, tty->row, entry);
			}

			tty->column += 1;

			if (g_active_tty == id) {
				vga_set_cursor_position(tty->column, tty->row);
			}
		}
	} break;

	case ANSI_STATE_ESC: {
		if (c == '[') {
			tty->ansi_state = ANSI_STATE_CSI;
			tty->ansi_param = 0;
		} else {
			tty->ansi_state = ANSI_STATE_NORMAL;
		}
	} break;

	case ANSI_STATE_CSI: {
		if (c >= '0' && c <= '9') {
			tty->ansi_param = tty->ansi_param * 10 + (c - '0');
		} else if (c == 'm') {
			if (tty->ansi_param == 0) {
				tty->color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
			} else if ((tty->ansi_param >= 30 && tty->ansi_param <= 37) || (tty->ansi_param >= 90 && tty->ansi_param <= 97)) {
				tty->color = vga_entry_color(ansi_to_vga(tty->ansi_param), vga_color_get_bg(tty->color));
			} else if ((tty->ansi_param >= 40 && tty->ansi_param <= 47) || (tty->ansi_param >= 100 && tty->ansi_param <= 107)) {
				tty->color = vga_entry_color(vga_color_get_fg(tty->color), ansi_to_vga(tty->ansi_param - 10));
			}

			tty->ansi_state = ANSI_STATE_NORMAL;
		}
	} break;
	}
}

void tty_putstr(tty_id_t id, const char *str) {
	k_size_t i = 0;
	while (str[i]) {
		tty_putchar(id, str[i]);
		i += 1;
	}
}

void tty_putchar_at(tty_id_t id, char c, int col, int row) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (col < 0 || col >= VGA_WIDTH) {
		return;
	}
	if (row < 0 || row >= VGA_HEIGHT) {
		return;
	}

	uint16_t entry = vga_entry(c, tty->color);

	tty->screen_buff[row * VGA_WIDTH + col] = entry;

	if (g_active_tty == id) {
		vga_set_entry_at(col, row, entry);
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

void tty_show_cursor(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	tty->cursor_visible = true;

	if (g_active_tty == id) {
		vga_show_cursor();
	}
}

void tty_hide_cursor(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	tty->cursor_visible = false;

	if (g_active_tty == id) {
		vga_hide_cursor();
	}
}

void tty_set_cursor_position(tty_id_t id, int col, int row) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

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

	tty->column = col;
	tty->row = row;

	if (g_active_tty == id) {
		vga_set_cursor_position(col, row);
	}
}

void tty_get_cursor_position(tty_id_t id, int *col, int *row) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (col) {
		*col = tty->column;
	}
	if (row) {
		*row = tty->row;
	}
}

void tty_move_cursor_left(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->column > 0) {
		tty->column -= 1;
	}

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_move_cursor_left_wrap(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->column > 0) {
		tty->column -= 1;
	} else if (tty->row > 0) {
		tty->row -= 1;
		tty->column = VGA_WIDTH - 1;
	}

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_move_cursor_right(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->column < VGA_WIDTH - 1) {
		tty->column += 1;
	}

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_move_cursor_right_wrap(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->column < VGA_WIDTH - 1) {
		tty->column += 1;
	} else if (tty->row < VGA_HEIGHT - 1) {
		tty->row += 1;
		tty->column = 0;
	}

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_move_cursor_up(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->row > 0) {
		tty->row -= 1;
	}

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_move_cursor_down(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	if (tty->row < VGA_HEIGHT - 1) {
		tty->row += 1;
	}

	if (g_active_tty == id) {
		vga_set_cursor_position(tty->column, tty->row);
	}
}

void tty_clear_char(tty_id_t id) {
	tty_t *tty = get_tty(id);
	if (!tty) {
		return;
	}

	uint16_t entry = vga_entry(0, tty->color);
	tty->screen_buff[tty->row * VGA_WIDTH + tty->column] = entry;

	if (g_active_tty == id) {
		vga_set_entry_at(tty->column, tty->row, entry);
	}
}
