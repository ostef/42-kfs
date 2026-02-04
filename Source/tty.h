#ifndef TTY_H
#define TTY_H

typedef enum {
    ANSI_STATE_NORMAL,
    ANSI_STATE_ESC,
    ANSI_STATE_CSI
} ansi_state_t;

void tty_initialize(void);
void tty_putchar(char c);
void tty_clear(void);
void tty_scroll_down(void);

void tty_putstr(const char* str); // tmp

#endif // TTY_H
