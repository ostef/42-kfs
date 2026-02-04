#include "LibKernel/libkernel.h"
#include "tty.h"

void kernel_main(void) {
    terminal_initialize();

    k_printf("Hello Kernel!\nkernel_main=%p\n", kernel_main);
}