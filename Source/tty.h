#ifndef TTY_H
#define TTY_H

#include "libkernel.h"

#define MAX_TTYS 10

typedef int32_t tty_id_t;

void tty_initialize(void);
void tty_set_active(tty_id_t id);
tty_id_t tty_get_active(void);

void tty_scroll_up(tty_id_t id);
void tty_clear(tty_id_t id);
void tty_putchar(tty_id_t id, char c);
void tty_putstr(tty_id_t id, const char *str);
void tty_putchar_at(tty_id_t id, char c, int col, int row);
void tty_clear_char(tty_id_t id);
void tty_set_color(tty_id_t id, uint8_t color);
uint8_t tty_get_color(tty_id_t id);

void tty_show_cursor(tty_id_t id);
void tty_hide_cursor(tty_id_t id);
void tty_set_cursor_position(tty_id_t id, int col, int row);
void tty_get_cursor_position(tty_id_t id, int *col, int *row);
void tty_move_cursor_left(tty_id_t id);
void tty_move_cursor_left_wrap(tty_id_t id);
void tty_move_cursor_right(tty_id_t id);
void tty_move_cursor_right_wrap(tty_id_t id);
void tty_move_cursor_up(tty_id_t id);
void tty_move_cursor_down(tty_id_t id);

#endif // TTY_H
