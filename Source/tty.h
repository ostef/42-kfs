#ifndef TERMINAL_H
#define TERMINAL_H

typedef enum {
    ANSI_STATE_NORMAL,
    ANSI_STATE_ESC,
    ANSI_STATE_CSI
} ansi_state_t;

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_clear(void);
void terminal_scroll_down(void);

void terminal_putstr(const char* str); // tmp

#endif // TERMINAL_H
