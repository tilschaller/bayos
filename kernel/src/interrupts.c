#include <interrupts.h>
#include <types/types.h>
#include <stdio.h>

typedef struct {
	uint16_t isr_low;
	uint16_t kernel_cs;
	uint8_t ist;
	uint8_t attr;
	uint16_t isr_mid;
	uint32_t isr_high;
	uint32_t reserved;
} __attribute__((packed)) idt_entry;

__attribute__((aligned(0x10)))
static idt_entry idt[256];

typedef struct {
	uint16_t size;
	idt_entry *idt;
} __attribute__((packed)) idtr;

static idtr idtr_val;

/*
	general exception handler
*/
__attribute__((noreturn))
void exception_handler(uint64_t int_num) {
	(void)int_num;
	printf("\nInterrupt %llu occured\nHalting execution\n", int_num);
	asm volatile("cli; hlt");
	__builtin_unreachable();
}

void idt_set_descriptor(uint8_t entry, uint64_t isr, uint8_t flags, uint8_t ist) {
	idt[entry].isr_low = isr & 0xffff;
	idt[entry].kernel_cs = 0x8;
	idt[entry].ist = ist;
	idt[entry].attr = flags;
	idt[entry].isr_mid = (isr >> 16) & 0xffff;
	idt[entry].isr_high = (isr >> 32) & 0xffffffff;
	idt[entry].reserved = 0;
}

/*
	defined in exception_handler.asm
*/
extern uint64_t isr_stub_table[32];
extern uintptr_t apic_error_handler;
extern uintptr_t spurious_handler;
extern uintptr_t timer_handler;

void int_init(void) {
	/*
		define the idtr
	*/
	idtr_val.size = sizeof(idt) - 1;
	idtr_val.idt = idt;

	/*
		set entries for the first 32 interrupts
		since they are not user defined but declared by the 
		manual
	*/
	for (uint8_t i = 0; i < 32; i++) {
		if (i == 0x8) {
			// double fault
			// set ist to entry one of tss
			// double fault stack
			// else set to 0 -> normal stack
			idt_set_descriptor(i, isr_stub_table[i], 0x8e, 1);
		} else {
			idt_set_descriptor(i, isr_stub_table[i], 0x8e, 0);
		}
	}

	idt_set_descriptor(32, timer_handler, 0x8e, 0);
	idt_set_descriptor(34, apic_error_handler, 0x8e, 0);
	idt_set_descriptor(0xff, spurious_handler, 0x8e, 0);

	asm volatile("lidt %0" : : "m"(idtr_val) : "memory");
}
