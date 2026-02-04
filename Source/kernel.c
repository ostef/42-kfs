#include "LibKernel/libkernel.h"
#include "terminal.h"

void kernel_main(void) {
    terminal_initialize();

    k_printf("Hello Kernel!\nkernel_main=%p\n", kernel_main);
}