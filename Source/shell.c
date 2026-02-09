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

static int shell_prompt(char *buff) {
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

	k_memcpy(buff, g_shell_text_buffer, g_shell_text_length);
	int len = g_shell_text_length;

	text_buffer_reset();

	return len;
}

static void get_next_arg(const char *buff, k_size_t buff_len, k_size_t *inout_offset, k_size_t *out_len) {
	k_size_t i = inout_offset ? *inout_offset : 0;
	k_size_t len = 0;

	while (i < buff_len && k_is_space(buff[i])) {
		i += 1;
	}

	while (i + len < buff_len && !k_is_space(buff[i + len])) {
		len += 1;
	}

	if (inout_offset) {
		*inout_offset = i;
	}
	if (out_len) {
		*out_len = len;
	}
}

void shell_loop() {
	char buff[k_array_count(g_shell_text_buffer)];
	while (true) {
		int len = shell_prompt(buff);

		k_size_t cmd_idx = 0, cmd_len = 0;
		get_next_arg(buff, len, &cmd_idx, &cmd_len);
		const char *cmd = buff + cmd_idx;

		if (cmd_len >= k_strlen("echo") && k_strncmp(cmd, "echo", cmd_len) == 0) {
			bool first = true;
			k_size_t arg_idx = cmd_idx + cmd_len, arg_len = 0;
			while (arg_idx < len) {
				if (!first) {
					k_printf(" ");
				}

				first = false;

				get_next_arg(buff, len, &arg_idx, &arg_len);
				k_printf("%S", arg_len, buff + arg_idx);

				arg_idx += arg_len;
			}

			k_printf("\n");
		} else if (cmd_len >= k_strlen("clear") && k_strncmp(cmd, "clear", cmd_len) == 0) {
			tty_clear(0);
		} else if (cmd_len >= k_strlen("halt") && k_strncmp(cmd, "halt", cmd_len) == 0) {
			k_printf("Halt\n");
			return;
		} else if (cmd_len > 0) {
			k_printf("Error: unknown command '%S'\n", cmd_len, cmd);
		}
	}
}
