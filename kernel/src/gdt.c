#include <types/types.h>
#include <gdt.h>

#define DOUBLE_FAULT_IST_INDEX 0

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed)) gdt_entry;

typedef struct {
	gdt_entry gdt_entry;
	uint8_t base_mid;
	uint32_t base_high;
	uint32_t reserved;
} __attribute__((packed)) gdt_tss_entry;

#define GDT_ENTRY(access_byte) 		\
	(gdt_entry) { 			\
		.limit_low = 0, 	\
		.base_low = 0, 		\
		.base_mid = 0, 		\
		.access = access_byte, 	\
		.granularity = 0, 	\
		.base_high = 0, 	\
	}

gdt_entry gdt[] = {
	GDT_ENTRY(0),
	GDT_ENTRY(0x9a),
	GDT_ENTRY(0x92),
	GDT_ENTRY(0xfa),
	GDT_ENTRY(0xf2),
};

typedef struct {
	uint16_t size;
	gdt_entry *gdt;
} __attribute((packed)) gdtr;

gdtr gdtr_val = {
	sizeof(gdt) - 1,
	gdt,
};

void gdt_init(void) {
	asm volatile("lgdt %0" : : "m"(gdtr_val) : "memory");
	/*
		Note: we dont need to reload the segment registers,
		because the bootloader uses the same entries
	*/
}
