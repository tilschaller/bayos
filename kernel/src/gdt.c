#include <types/types.h>
#include <gdt.h>
#include <string.h>

#define DOUBLE_FAULT_IST_INDEX 0

uint8_t double_fault_stack[0x1000];
uint8_t kernel_stack[0x1000];

typedef struct {
	uint32_t reserved0;
	// consists of high & low
	uint64_t rsp[3];
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iopb;
} __attribute__((packed)) tss;

tss tss_val = {0};

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t base_mid;
	uint8_t access;
	uint8_t granularity;
	uint8_t base_high;
} __attribute__((packed)) gdt_entry;

typedef struct {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t  base_mid1;
	uint8_t  access;
	uint8_t  granularity;
	uint8_t  base_mid2;
	uint32_t base_high;
	uint32_t reserved;
} __attribute__((packed)) gdt_tss_entry;

#define GDT_ENTRY(access_byte, flags)   \
        (gdt_entry) {                   \
                .limit_low = 0xffff,    \
                .base_low = 0,          \
                .base_mid = 0,          \
                .access = access_byte,  \
                .granularity = (flags << 4) | 0xf,   \
                .base_high = 0,         \
        }

#define GDT_TSS_ENTRY(addr)                                                     \
        (gdt_tss_entry) {                                                       \
                .limit_low = sizeof(tss) - 1,                                   \
                .base_low = (uint16_t)((uint64_t)(addr) & 0xffff),              \
                .base_mid1 = (uint8_t)(((uint64_t)(addr) >> 16) & 0xff),        \
                .access = 0x89,                                                 \
                .granularity = 0,				                \
                .base_mid2 = (uint8_t)(((uint64_t)(addr) >> 24) & 0xff),        \
                .base_high = (uint32_t)(((uint64_t)(addr) >> 32) & 0xffffffff), \
                .reserved = 0,                                                  \
        }

typedef struct {
	gdt_entry null;
	gdt_entry kernel_code;
	gdt_entry kernel_data;
	gdt_entry user_code;
	gdt_entry user_data;
	gdt_tss_entry tss;
} __attribute__((packed)) gdt;


typedef struct {
	uint16_t size;
	gdt *gdt;
} __attribute((packed)) gdtr;

gdt gdt_val = {
	{0},
	GDT_ENTRY(0x9a, 0xa),
	GDT_ENTRY(0x92, 0xc),
	GDT_ENTRY(0xfa, 0xa),
	GDT_ENTRY(0xf2, 0xc),
	{0},
};

gdtr gdtr_val = {
	sizeof(gdt) - 1,
	&gdt_val,
};

void gdt_init(void) {
	/*
	        initialize tss
	*/
	tss_val.rsp[0] = (uint64_t)kernel_stack + sizeof(kernel_stack);
	tss_val.ist[DOUBLE_FAULT_IST_INDEX] = (uint64_t)double_fault_stack + sizeof(double_fault_stack);

	gdt_tss_entry tss = GDT_TSS_ENTRY(&tss_val);
	memcpy(&gdt_val.tss, &tss, sizeof(tss));

	/*
	        load gdt
	        it is initialized at compile time
	*/
	asm volatile("lgdt %0" : : "m"(gdtr_val) : "memory");
	/*
	        NOTE: reloading segment registers is not required
	        since the bootloader uses the same entries
	*/
	/*
	        load tss
	*/
	asm volatile("ltr %0" : : "r"((uint16_t)0x28) : );
}
