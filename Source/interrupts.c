#include "interrupts.h"
#include "ioport.h"

static idt_gate_t g_idt[NUM_IDT_GATES];
static idt_register_t g_idt_register;
static isr_handler_t g_irq_handlers[256];

typedef uint8_t pic_port_t;
enum {
	PIC1_CMD  = 0x20,
	PIC1_DATA = 0x21,
	PIC2_CMD  = 0xa0,
	PIC2_DATA = 0xa1,
};

enum {
	PIC_CMD_INITIALIZE = 0x10,
	PIC_CMD_USE_ICW4 = 0x01, // ICW = Initialization Command Word
	PIC_CMD_END_OF_INTERRUPT = 0x20,
	PIC_CMD_READ_INTERRUPT_REQUEST_REGISTER = 0x0a,
	PIC_CMD_READ_IN_SERVICE_REGISTER = 0x0b,
};

enum {
	PIC_DATA_8086_MODE = 0x01,
};

void idt_set_gate(int32_t index, uint32_t handler_address) {
	if (index < 0 || index >= NUM_IDT_GATES) {
		return;
	}

	g_idt[index] = (idt_gate_t){};
	g_idt[index].handler_address_low = LOW_16BITS(handler_address);
	g_idt[index].handler_address_high = HIGH_16BITS(handler_address);
	g_idt[index].segment_selector = KERNEL_CODE_SEGMENT;
	g_idt[index].flags.present = 1;
	g_idt[index].flags.privilege_level = 0;
	g_idt[index].flags.gate_type = IDT_GATE_32BIT_INTERRUPT;
}

void idt_load_register()  {
	g_idt_register.offset = (uint32_t)&g_idt;
	g_idt_register.size = sizeof(g_idt) - 1;

	asm volatile("lidtl (%0)" : : "r"(&g_idt_register));
	asm volatile("sti");
}

void interrupts_initialize() {
    idt_set_gate(0, (uint32_t)isr0);
    idt_set_gate(1, (uint32_t)isr1);
    idt_set_gate(2, (uint32_t)isr2);
    idt_set_gate(3, (uint32_t)isr3);
    idt_set_gate(4, (uint32_t)isr4);
    idt_set_gate(5, (uint32_t)isr5);
    idt_set_gate(6, (uint32_t)isr6);
    idt_set_gate(7, (uint32_t)isr7);
    idt_set_gate(8, (uint32_t)isr8);
    idt_set_gate(9, (uint32_t)isr9);
    idt_set_gate(10, (uint32_t)isr10);
    idt_set_gate(11, (uint32_t)isr11);
    idt_set_gate(12, (uint32_t)isr12);
    idt_set_gate(13, (uint32_t)isr13);
    idt_set_gate(14, (uint32_t)isr14);
    idt_set_gate(15, (uint32_t)isr15);
    idt_set_gate(16, (uint32_t)isr16);
    idt_set_gate(17, (uint32_t)isr17);
    idt_set_gate(18, (uint32_t)isr18);
    idt_set_gate(19, (uint32_t)isr19);
    idt_set_gate(20, (uint32_t)isr20);
    idt_set_gate(21, (uint32_t)isr21);
    idt_set_gate(22, (uint32_t)isr22);
    idt_set_gate(23, (uint32_t)isr23);
    idt_set_gate(24, (uint32_t)isr24);
    idt_set_gate(25, (uint32_t)isr25);
    idt_set_gate(26, (uint32_t)isr26);
    idt_set_gate(27, (uint32_t)isr27);
    idt_set_gate(28, (uint32_t)isr28);
    idt_set_gate(29, (uint32_t)isr29);
    idt_set_gate(30, (uint32_t)isr30);
    idt_set_gate(31, (uint32_t)isr31);

	// https://en.wikibooks.org/wiki/X86_Assembly/Programmable_Interrupt_Controller
	// The PIC is the microcontroller that handles incoming interrupts from hardware internal and external peripherals
	// (timers, keyboards, floppy disk controllers...) and passes them to the CPU.
	// The PIC, identifies interrupts by a number, and upon receiving that interrupt, adds an
	// offset to it (by default 8 for PIC1) before passing it to the CPU.
	// The default vector offset of PIC1 is 8, so it uses interrupt numbers 8 to 15.
	// This conflicts with protected mode CPU interrupts, which go from 0 to 31, so we need
	// to remap the PICs interrupt numbers to start at 32. If this sounds stupid, it is because
	// it's a design mistake by IBM who used reserved interrupt numbers for IRQs :/
	ioport_write_byte(PIC1_CMD, PIC_CMD_INITIALIZE | PIC_CMD_USE_ICW4);
	ioport_write_byte(PIC2_CMD, PIC_CMD_INITIALIZE | PIC_CMD_USE_ICW4);
	// ICW2
	ioport_write_byte(PIC1_DATA, PIC1_VECTOR_OFFSET);
	ioport_write_byte(PIC2_DATA, PIC2_VECTOR_OFFSET);
	// ICW3
	// This lets PIC1 and PIC2 know of each other, though I don't fully understand the values
	ioport_write_byte(PIC1_DATA, 0x4);
	ioport_write_byte(PIC2_DATA, IRQ_BASE_INDEX_PIC2_TO_PIC1);
	// ICW4
	ioport_write_byte(PIC1_DATA, PIC_DATA_8086_MODE);
	ioport_write_byte(PIC2_DATA, PIC_DATA_8086_MODE);
	// Initialization done!

	// Allow receiving all IRQs
	ioport_write_byte(PIC1_DATA, 0);
	ioport_write_byte(PIC2_DATA, 0);

	idt_set_gate(PIC1_VECTOR_OFFSET + 0, (uint32_t)irq0);
    idt_set_gate(PIC1_VECTOR_OFFSET + 1, (uint32_t)irq1);
    idt_set_gate(PIC1_VECTOR_OFFSET + 2, (uint32_t)irq2);
    idt_set_gate(PIC1_VECTOR_OFFSET + 3, (uint32_t)irq3);
    idt_set_gate(PIC1_VECTOR_OFFSET + 4, (uint32_t)irq4);
    idt_set_gate(PIC1_VECTOR_OFFSET + 5, (uint32_t)irq5);
    idt_set_gate(PIC1_VECTOR_OFFSET + 6, (uint32_t)irq6);
    idt_set_gate(PIC1_VECTOR_OFFSET + 7, (uint32_t)irq7);
    idt_set_gate(PIC1_VECTOR_OFFSET + 8, (uint32_t)irq8);
    idt_set_gate(PIC1_VECTOR_OFFSET + 9, (uint32_t)irq9);
    idt_set_gate(PIC1_VECTOR_OFFSET + 10, (uint32_t)irq10);
    idt_set_gate(PIC1_VECTOR_OFFSET + 11, (uint32_t)irq11);
    idt_set_gate(PIC1_VECTOR_OFFSET + 12, (uint32_t)irq12);
    idt_set_gate(PIC1_VECTOR_OFFSET + 13, (uint32_t)irq13);
    idt_set_gate(PIC1_VECTOR_OFFSET + 14, (uint32_t)irq14);
    idt_set_gate(PIC1_VECTOR_OFFSET + 15, (uint32_t)irq15);

	idt_load_register();

	k_printf("Initialized interrupt handlers\n");
}

static const char *g_exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",

    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",

    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void handle_isr(isr_registers_t registers) {
    if (registers.interrupt_number < (uint32_t)k_array_count(g_exception_messages)) {
        k_printf("Received interrupt %u, error code %x: %s\n", registers.interrupt_number, registers.error_code, g_exception_messages[registers.interrupt_number]);
    } else {
        k_printf("Received interrupt %u, error code %x\n", registers.interrupt_number, registers.error_code);
    }

    k_pseudo_breakpoint();
}

void isr_register_handler(uint8_t index, isr_handler_t handler) {
	g_irq_handlers[index] = handler;
}

// https://wiki.osdev.org/8259_PIC
static
uint16_t get_pic_in_service_register() {
	ioport_write_byte(PIC1_CMD, PIC_CMD_READ_IN_SERVICE_REGISTER);
	ioport_write_byte(PIC2_CMD, PIC_CMD_READ_IN_SERVICE_REGISTER);
	uint8_t pic1 = ioport_read_byte(PIC1_CMD);
	uint8_t pic2 = ioport_read_byte(PIC2_CMD);

	return ((uint16_t)pic2 < 8) | ((uint16_t)pic1);
}

void handle_irq(isr_registers_t registers) {
    // Handle spurious "fake" IRQs. See https://wiki.osdev.org/8259_PIC#Spurious_IRQs
	uint8_t pic_interrupt_number = registers.interrupt_number - PIC1_VECTOR_OFFSET;
	if (pic_interrupt_number == 7 || pic_interrupt_number == 15) {
		uint16_t pic_interrupt_flag = 1 << (uint16_t)pic_interrupt_number;
		uint16_t isr = get_pic_in_service_register();

		if ((isr & pic_interrupt_flag) == 0) { // ISR flag for the interrupt is not set, this is a spurious IRQ
			// If the spurious IRQ came from PIC2, we still need to send PIC1 an EOI
			if (pic_interrupt_number == 15) {
				ioport_write_byte(PIC1_CMD, PIC_CMD_END_OF_INTERRUPT);
			}

			return;
		}
	}

	// Send EOI command to the PICs so we can receive further IRQs later
	// For IRQs that came from PIC2, we need to send this command to PIC1 and PIC2. Otherwise, we send it only to PIC1
	// Maybe we should do that after we've called the handler?
	if (registers.interrupt_number >= PIC2_VECTOR_OFFSET) {
		ioport_write_byte(PIC2_CMD, PIC_CMD_END_OF_INTERRUPT);
	}
	ioport_write_byte(PIC1_CMD, PIC_CMD_END_OF_INTERRUPT);

	if (g_irq_handlers[registers.interrupt_number] != 0) {
		isr_handler_t handler = g_irq_handlers[registers.interrupt_number];
		handler(registers);
	}
}

