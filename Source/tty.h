#ifndef tty_H
#define tty_H

#include "LibKernel/libkernel.h"

typedef enum {
    ANSI_STATE_NORMAL,
    ANSI_STATE_ESC,
    ANSI_STATE_CSI
} ansi_state_t;

void	tty_initialize(void);
void	tty_putchar(char c);
void	tty_clear(void);
void	tty_scroll_down(void);
k_size	tty_putstr(const char* str);


#endif // tty_H
