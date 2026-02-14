#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include "libkernel.h"

// Vocabulary dump:
// IDT: Interrupt Descriptor Table
// IDT Gate: entry in the IDT
// IDT Register: data describing the IDT
// ISR: Interrupt Service Routine (i.e. CPU raised interrupts)
// IRQ: Interrupt ReQuest (i.e. hardware raised interrupts, handled by the PICs)
// PIC: Programmable Interrupt Controller

void interrupts_initialize();

enum {
	IDT_GATE_TASK = 0x5,
	IDT_GATE_16BIT_INTERRUPT = 0x6,
	IDT_GATE_16BIT_TRAP = 0x7,
	IDT_GATE_32BIT_INTERRUPT = 0xe,
	IDT_GATE_32BIT_TRAP = 0xf,
};

typedef struct idt_gate_flags_t {
	uint8_t gate_type       : 4;
	uint8_t unused          : 1;
	uint8_t privilege_level : 2;
	uint8_t present         : 1;
} __attribute__((packed)) idt_gate_flags_t;

typedef struct idt_gate_t {
	uint16_t handler_address_low;
	uint16_t segment_selector;
	uint8_t unused; // Always set to 0
	idt_gate_flags_t flags;
	uint16_t handler_address_high;
} __attribute__((packed)) idt_gate_t;

typedef struct idt_register_t {
	uint16_t size;
	uint32_t offset;
} __attribute__((packed)) idt_register_t;

#define NUM_IDT_GATES 256

void idt_get_gate(int32_t index, uint32_t handler_address);
void idt_load_register();

typedef struct interrupt_registers_t {
	uint32_t ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
	uint32_t interrupt_number, error_code;
	uint32_t eip, cs, eflags, useresp, ss;
} interrupt_registers_t;

typedef void (*interrupt_handler_t)(interrupt_registers_t);

void print_interrupt_registers(interrupt_registers_t registers);
void interrupt_register_handler(uint8_t index, interrupt_handler_t handler);

// Must be divisible by 8!
#define PIC1_VECTOR_OFFSET 32
#define PIC2_VECTOR_OFFSET 40

enum {
	INT_DIVISION_BY_ZERO = 0,
	INT_STEP = 1,
	INT_NON_MASKABLE_INTERRUPT = 2,
	INT_BREAKPOINT = 3,
	INT_OVERFLOW = 4,
	INT_BOUNDS_CHECK_FAILURE = 5,
	INT_INVALID_OPCODE = 6,
	INT_NO_COPROCESSOR = 7,
	INT_DOUBLE_FAULT = 8,
	INT_COPROCESSOR_SEGMENT_OVERRUN = 9,
	INT_INVALID_TASK_STATE_SEGMENT = 10,
	INT_SEGMENT_NOT_PRESENT = 11,
	INT_STACK_FAULT = 12,
	INT_GENERAL_PROTECTION_FAULT = 13,
	INT_PAGE_FAULT = 14,
	INT_X87_FPU_ERROR = 16,
	INT_ALIGNMENT_CHECK_FAILURE = 17,
	INT_MACHINE_CHECK_FAILURE = 18,
	INT_SIMD_FPU_EXCEPTION = 19,
};

enum {
	IRQ_BASE_INDEX_TIMER = 0,
	IRQ_BASE_INDEX_KEYBOARD_CONTROLLER = 1,
	IRQ_BASE_INDEX_PIC2_TO_PIC1 = 2,
	IRQ_BASE_INDEX_COM2 = 3,
	IRQ_BASE_INDEX_COM1 = 4,
	IRQ_BASE_INDEX_TERMINAL2 = 5,
	IRQ_BASE_INDEX_FLOPPY_CONTROLLER = 6,
	IRQ_BASE_INDEX_TERMINAL1 = 7,
	IRQ_BASE_INDEX_RTC_TIMER = 8,
	IRQ_BASE_INDEX_ACPI = 9, // https://en.wikibooks.org/wiki/X86_Assembly/ACPI
	IRQ_BASE_INDEX_MOUSE_CONTROLLER = 12,
	IRQ_BASE_INDEX_MATH_COPROCESSOR = 13,
	IRQ_BASE_INDEX_ATA_CHANNEL1 = 14,
	IRQ_BASE_INDEX_ATA_CHANNEL2 = 15,
};

enum {
	IRQ_INDEX_TIMER = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_TIMER,
	IRQ_INDEX_KEYBOARD_CONTROLLER = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_KEYBOARD_CONTROLLER,
	IRQ_INDEX_PIC2_TO_PIC1 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_PIC2_TO_PIC1,
	IRQ_INDEX_COM2 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_COM2,
	IRQ_INDEX_COM1 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_COM1,
	IRQ_INDEX_TERMINAL2 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_TERMINAL2,
	IRQ_INDEX_FLOPPY_CONTROLLER = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_FLOPPY_CONTROLLER,
	IRQ_INDEX_TERMINAL1 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_TERMINAL1,
	IRQ_INDEX_RTC_TIMER = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_RTC_TIMER,
	IRQ_INDEX_ACPI = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_ACPI,
	IRQ_INDEX_MOUSE_CONTROLLER = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_MOUSE_CONTROLLER,
	IRQ_INDEX_MATH_COPROCESSOR = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_MATH_COPROCESSOR,
	IRQ_INDEX_ATA_CHANNEL1 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_ATA_CHANNEL1,
	IRQ_INDEX_ATA_CHANNEL2 = PIC1_VECTOR_OFFSET + IRQ_BASE_INDEX_ATA_CHANNEL2,
};

// Defined in interrupt_handlers.asm
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

#endif // INTERRUPTS_H