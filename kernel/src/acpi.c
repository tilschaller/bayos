#include <acpi.h>
#include <types/types.h>
#include <memory.h>
#include <string.h>
#include <fail.h>
#include <stdio.h>
#include <ports.h>

uint32_t *lapic = 0;

inline void send_eoi(void) {
	lapic[44] = 0;
}

typedef struct {
	char signature[8];
	uint8_t checksum;
	char oemid[6];
	uint8_t revision;
	uint32_t rsdt_addr;
	uint32_t length;
	uint64_t xsdt_addr;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__((packed)) xsdp;

typedef struct {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oemid[6];
	char oemtableid[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
} __attribute__((packed)) acpi_header;

typedef struct {
	acpi_header header;
	uint32_t pointer[];
} __attribute__((packed)) rsdt;

typedef struct {
	acpi_header header;
	uint32_t pointer[];
} __attribute__((packed)) xsdt;

typedef struct {
	acpi_header header;
	uint32_t lapic_addr;
	uint32_t flags;
	// madt_entry_x entries[]
} __attribute__((packed)) madt;

typedef struct {
	uint8_t type; // is 1
	uint8_t length;
	uint8_t id;
	uint8_t reserved;
	uint32_t addr;
	uint32_t global_system_interrupt_base;
} __attribute__((packed)) madt_entry_io_apic;

madt *init_acpi_1(xsdp *rsdp) {
	rsdt *rsdt = phys_to_virt(rsdp->rsdt_addr);
	if (rsdp->rsdt_addr == 0 || rsdt == NULL || memcmp(rsdt->header.signature, "RSDT", 4) != 0) {
		printf("Could not find rsdt\n");
		hcf();
	}

	madt *madt;
	for (int i = 0; i < (int)((rsdt->header.length - sizeof(acpi_header)) / 4); i++) {
		acpi_header *header = phys_to_virt(rsdt->pointer[i]);
		if (header == NULL) continue;
		int res = memcmp(header->signature, "APIC", 4);
		if (res == 0) {
			madt = (void*)header;
			break;
		}
	}
	if (madt == NULL) {
		printf("Could not find madt\n");
		hcf();
	}

	lapic = phys_to_virt(madt->lapic_addr);

	return madt;
}

madt* init_acpi_2(xsdp *xsdp) {
	xsdt *xsdt = phys_to_virt(xsdp->xsdt_addr);
	if (xsdp->xsdt_addr == 0 || xsdt == NULL || memcmp(xsdt->header.signature, "XSDT", 4) != 0) {
		printf("Could not find xsdt\n");
		hcf();
	}

	madt *madt;

	for (int i = 0; i < (int)((xsdt->header.length - sizeof(acpi_header)) / 8); i++) {
		acpi_header *header = phys_to_virt(xsdt->pointer[i]);
		if (header == NULL) continue;
		int res = memcmp(header->signature, "APIC", 4);
		if (res == 0) {
			madt = (void*)header;
			break;
		}
	}
	if (madt == NULL) {
		printf("Could not find madt\n");
		hcf();
	}

	lapic = phys_to_virt(madt->lapic_addr);

	return madt;
}

void acpi_init(void) {
	// first disale the legacy pic
#define PIC_1_CMD_PORT 0x20
#define PIC_1_DAT_PORT 0x21
#define PIC_2_CMD_PORT 0xa0
#define PIC_2_DAT_PORT 0xa1
	write_port_u8(PIC_1_CMD_PORT, 0x11);
	write_port_u8(PIC_2_CMD_PORT, 0x11);

	write_port_u8(PIC_1_DAT_PORT, 0xf8);
	write_port_u8(PIC_2_DAT_PORT, 0xf0);

	write_port_u8(PIC_1_DAT_PORT, 0x4);
	write_port_u8(PIC_2_DAT_PORT, 0x2);
	
	write_port_u8(PIC_1_DAT_PORT, 0x1);
	write_port_u8(PIC_2_DAT_PORT, 0x1);

	write_port_u8(PIC_1_DAT_PORT, 0xff);
	write_port_u8(PIC_2_DAT_PORT, 0xff);

	// find the rsdp in bios area
	// 0xe0000 - 0xfffff
	// just hope no gargabe memory has signature
	xsdp *rsdp = 0;
	for (uintptr_t i = 0xe0000; i <= 0xfffff; i += 16) {
		int res = memcmp(phys_to_virt(i), "RSD PTR ", 8);
		if (res == 0) {
			rsdp = phys_to_virt(i);
			break;
		}
	}
	if (rsdp == NULL) {
		printf("Could not find rsdp\n");
		hcf();
	}

	madt *madt = NULL;
	if (rsdp->revision == 0) {
		madt = init_acpi_1(rsdp);
	} else {
		madt = init_acpi_2(rsdp);
	}

	if (lapic == 0) {
		printf("Could not find lapic\n");
		hcf();
	}
	
	// init lapic to well known state
	lapic[56] = 0xffffffff;
	lapic[52] = (lapic[52] & 0xffffff) | 1;
	lapic[200] = 0x10000;
	lapic[208] = (4 << 8) | (1 << 16);
	lapic[212] = 0x10000;
	lapic[216] = 0x10000;
	lapic[32] = 0;

	// set lapic internal error vector
	lapic[220] = 34 | 0x100; 
	// set spurious interrupt vector
	lapic[60] = 0xff | 0x100;

	// enable lapic timer
	lapic[200] = 32 | 0x100;
	lapic[248] = 0x3;

	// configure the pit
	write_port_u8(0x61, (read_port_u8(0x61) & 0xfd) | 1);
	write_port_u8(0x43, 0b10110010);
	write_port_u8(0x42, 0x9b);
	read_port_u8(0x60);
	write_port_u8(0x42, 0x2e);

	lapic[200] |= 0x10000;
	lapic[224] = 0xffffffff;

	uint8_t val = read_port_u8(0x61) & 0xfe;
	write_port_u8(0x61, val);
	write_port_u8(0x61, val | 1);

	while ((read_port_u8(0x61) & 0x20) == 0) {}

	lapic[200] = 0x10000;

	uint32_t ticks_in_10ms = 0xffffffff - lapic[228];

	lapic[200] = 32 | 0x20000;
	lapic[248] = 0x3;
	lapic[224] = ticks_in_10ms;

	uint32_t lapic_id = lapic[8];

	// configure io_apic
	// first find all io apics
	for (void *addr = (void*)madt + 0x2c; addr < (void*)madt + madt->header.length;) {
		madt_entry_io_apic *io_apic = (madt_entry_io_apic*)addr;
		if (io_apic->type == 1) {
			volatile uint16_t *ioregsel = phys_to_virt(io_apic->addr);
			volatile uint32_t *ioregwin = phys_to_virt(io_apic->addr + 16);

			*ioregsel = 1;
			uint8_t num_of_irqs = ((*ioregwin >> 16) & 0xff) - 1;

			for (uint16_t i = 0; i < num_of_irqs; i++) {
				*ioregsel = 0x10 + i * 2;
				*ioregwin = 1 << 16;
				*ioregsel = 0x11 + i * 2;
				*ioregwin = 0;
			}

			if (io_apic->global_system_interrupt_base == 0) {
				*ioregsel = 0x12;
				*ioregwin = 33;
				*ioregsel = 0x13;
				*ioregwin = lapic_id << 24;
			}
		}
		addr += io_apic->length;
	} 
}
