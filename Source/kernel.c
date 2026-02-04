#include "LibKernel/libkernel.h"
#include "tty.h"

void kernel_main(void) {
    terminal_initialize();

    k_print_str("Hello, Kernel World!\n");
    k_print_str("Hello, Kernel World!1\n");
}