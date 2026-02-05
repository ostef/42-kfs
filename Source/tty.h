#ifndef TTY_H
#define TTY_H

#include "libkernel.h"

typedef enum ansi_state_t {
    ANSI_STATE_NORMAL,
    ANSI_STATE_ESC,
    ANSI_STATE_CSI
} ansi_state_t;

void	tty_initialize(void);
void	tty_putchar(char c);
void	tty_clear(void);
void	tty_scroll_down(void);
k_size	tty_putstr(const char* str);


#endif // TTY_H
