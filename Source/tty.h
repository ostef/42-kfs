#ifndef TTY_H
#define TTY_H

#include "libkernel.h"

#define MAX_TTYS 10

typedef uint8_t ansi_state_t;
enum {
	ANSI_STATE_NORMAL,
	ANSI_STATE_ESC,
	ANSI_STATE_CSI
};

typedef int32_t tty_id_t;

void tty_initialize(void);
tty_id_t tty_create(void);
void tty_destroy(tty_id_t id);
void tty_clear(tty_id_t id);
void tty_scroll_up(tty_id_t id);
void tty_set_active(tty_id_t id);
tty_id_t tty_get_active(void);
void tty_putchar(tty_id_t id, char c);
void tty_set_color(tty_id_t id, uint8_t color);
uint8_t tty_get_color(tty_id_t id);
void tty_show_cursor(tty_id_t id);
void tty_hide_cursor(tty_id_t id);
void tty_set_cursor_position(tty_id_t id, int col, int row);
void tty_get_cursor_position(tty_id_t id, int *col, int *row);

#endif // TTY_H
