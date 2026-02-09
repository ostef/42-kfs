#include "shell.h"
#include "keyboard.h"
#include "tty.h"
#include "vga.h" // Ugh... We shouldn't need VGA specific stuff here...

static char g_shell_text_buffer[200];
static int g_shell_text_length;
static int g_shell_text_cursor;

static void text_move_cursor_left() {
	if (g_shell_text_cursor > 0) {
		g_shell_text_cursor -= 1;
		tty_move_cursor_left_wrap(0);
	}
}

static void text_move_cursor_right() {
	if (g_shell_text_cursor < g_shell_text_length) {
		g_shell_text_cursor += 1;
		tty_move_cursor_right_wrap(0);
	}
}

static void text_insert(char c) {
	if (g_shell_text_length >= k_array_count(g_shell_text_buffer)) {
		return;
	}

	int tty_col, tty_row;
	tty_get_cursor_position(0, &tty_col, &tty_row);

	for (int i = g_shell_text_length; i > g_shell_text_cursor; i -= 1) {
		g_shell_text_buffer[i] = g_shell_text_buffer[i - 1];
	}

	g_shell_text_length += 1;

	for (int i = g_shell_text_cursor + 1; i < g_shell_text_length; i += 1) {
		tty_col += 1;
		if (tty_col >= VGA_WIDTH) {
			tty_col = 0;
			tty_row += 1;

			if (tty_row >= VGA_HEIGHT) {
				tty_scroll_up(0);
				tty_row -= 1;
			}
		}

		tty_putchar_at(0, g_shell_text_buffer[i], tty_col, tty_row);
	}

	g_shell_text_buffer[g_shell_text_cursor] = c;
	g_shell_text_cursor += 1;

	tty_putchar(0, c);
}

static void text_delete() {
	if (g_shell_text_length <= 0 || g_shell_text_cursor >= g_shell_text_length) {
		return;
	}

	int tty_col, tty_row;
	tty_get_cursor_position(0, &tty_col, &tty_row);

	for (int i = g_shell_text_cursor; i < g_shell_text_length - 1; i += 1) {
		g_shell_text_buffer[i] = g_shell_text_buffer[i + 1];

		tty_putchar_at(0, g_shell_text_buffer[i + 1], tty_col, tty_row);

		tty_col += 1;
		if (tty_col >= VGA_WIDTH) {
			tty_col = 0;
			tty_row += 1;
		}
	}

	tty_putchar_at(0, 0, tty_col, tty_row);

	g_shell_text_length -= 1;
}

static void text_backspace() {
	if (g_shell_text_length <= 0 || g_shell_text_cursor <= 0) {
		return;
	}

	text_move_cursor_left();
	text_delete();
}

static void text_buffer_reset() {
	g_shell_text_length = 0;
	g_shell_text_cursor = 0;
}

static void shell_prompt() {
	tty_putstr(0, "$>");

	bool submitted = false;
	while (!submitted) {
		kb_event_t kb;
		if (kb_poll_event(&kb)) {
			switch (kb.type) {
			case KB_EVENT_TEXT_INPUT: {
				text_insert(kb.ascii_value);
			} break;

			case KB_EVENT_PRESS:
			case KB_EVENT_REPEAT: {
				if (kb_get_mod_state() == 0) {
					if (kb.scancode == KB_SCANCODE_LEFT) {
						text_move_cursor_left();
					} else if (kb.scancode == KB_SCANCODE_RIGHT) {
						text_move_cursor_right();
					} else if (kb.scancode == KB_SCANCODE_RETURN) {
						submitted = true;
					} else if (kb.scancode == KB_SCANCODE_BACKSPACE) {
						text_backspace();
					} else if (kb.scancode == KB_SCANCODE_DELETE) {
						text_delete();
					}
				}
			} break;
			}
		}
	}

	tty_putchar(0, '\n');
	text_buffer_reset();
}

void shell_loop() {
	while (true) {
		shell_prompt();
	}
}
