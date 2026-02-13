#include <interrupts.h>
#include <types/types.h>
#include <stdio.h>
#include <framebuffer.h>
#include <acpi.h>
#include <ports.h>
#include <keyboard.h>
#include <scheduler.h>
#include <fail.h>

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
static idt_entry idt[0x100];

typedef struct {
	uint16_t size;
	idt_entry *idt;
} __attribute__((packed)) idtr;

static idtr idtr_val;

/*
	general exception handler
*/
__attribute__((noreturn))
void exception_handler(uint64_t int_num, uint64_t error) {
	printf("Exception %llx occured\n", int_num);
	printf("Error Code: %llx\n", int_num);

	hcf();
}

void keyboard_handler(cpu_status_t *context) {
	(void)context;

	uint8_t c = get_key();

	if (c)
		putchar((int)c);

	send_eoi();
}

// time is in ms since boot, all timer interrupts should be 10ms apart, so we add 10 each interrupt
// NOTE: this is not that accurate
uint64_t time = 0;
cpu_status_t *timer_handler(cpu_status_t *old_context) {
	time += 10;

	cpu_status_t *new_context = schedule(old_context);

	send_eoi();

	return new_context;
}


void apic_error_handler(cpu_status_t *context) {
	(void)context;
	printf("Internal Apic error occured\n");
}

void spurious_handler(cpu_status_t *context) {
	(void)context;
}

void syscall_handler(cpu_status_t *context, uint64_t arg_1, uint64_t arg_2, uint64_t arg_3) {
	(void)context;
	if (arg_1 == 1) { // write syscall
		char *msg = (char*)arg_2;
		for (uint64_t i = 0; i < arg_3; i++) {
			putchar((int)*msg++);
		}
	} else if (arg_1 == 2) { // exit syscall
		exit();
	}
}

void idt_set_descriptor(uint8_t entry, uint64_t isr, uint8_t flags, uint8_t ist) {
	idt[entry].isr_low = isr & 0xffff;
	idt[entry].kernel_cs = 0x8;
	idt[entry].ist = ist & 0x7;
	idt[entry].attr = flags;
	idt[entry].isr_mid = (isr >> 16) & 0xffff;
	idt[entry].isr_high = (isr >> 32) & 0xffffffff;
	idt[entry].reserved = 0;
}

/*
	defined in exception_handler.asm
*/
extern uintptr_t isr_stub_table[32];
extern const void keyboard_handler_stub;
extern const void timer_handler_stub;
extern const void apic_error_handler_stub;
extern const void spurious_handler_stub;
extern const void syscall_handler_stub;

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

	idt_set_descriptor(32, (uintptr_t)&timer_handler_stub, 0x8e, 0);
	idt_set_descriptor(33, (uintptr_t)&keyboard_handler_stub, 0x8e, 0);
	idt_set_descriptor(34, (uintptr_t)&apic_error_handler_stub, 0x8e, 0);
	idt_set_descriptor(0xff, (uintptr_t)&spurious_handler_stub, 0x8e, 0);
	idt_set_descriptor(0x80, (uintptr_t)&syscall_handler_stub, 0xee, 0);

	asm volatile("lidt %0" : : "m"(idtr_val) : "memory");
}

int int_enabled  = 0;
inline void enable_interrupts(void) {
	int_enabled = 1;
	asm volatile("sti");
}
