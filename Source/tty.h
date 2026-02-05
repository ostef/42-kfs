#ifndef TTY_H
#define TTY_H

#include "libkernel.h"

typedef uint8_t ansi_state_t;
enum {
	ANSI_STATE_NORMAL,
	ANSI_STATE_ESC,
	ANSI_STATE_CSI
};

void	tty_initialize(void);
void	tty_putchar(char c);
void	tty_clear(void);
void	tty_scroll_down(void);

#endif // TTY_H
