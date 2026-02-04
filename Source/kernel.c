#include "LibKernel/libkernel.h"
#include "tty.h"

void kernel_main(void) {
    tty_initialize();

    k_printf("Hello Kernel!\nkernel_main=%p\n", kernel_main);
    tty_putstr("This is \x1b[31mred\x1b[0m text.\n");
    tty_putstr("This is \x1b[41mred\x1b[0m text.\n");
}