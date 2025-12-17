/*
this is not actually 32bit mode
but 16 bit code, but is named bootloader32
so it is linked after bootloader_16 is the boot binary
*/

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long int uint32_t;
typedef unsigned long long int uint64_t;
typedef uint32_t size_t;

typedef struct {
	uint8_t ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t phoff;
	uint64_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} __attribute__((packed)) elf_header;

typedef struct {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t vaddr;
	uint64_t paddr;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t align;
} __attribute__((packed)) elf_program_header;

__attribute__((section(".bootloader32")))
void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

__attribute__((section(".bootloader32")))
void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

__attribute__((section(".bootloader32")))
void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

__attribute__((section(".bootloader32")))
int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

__attribute__((section(".bootloader32"), noreturn, naked))
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

typedef struct {
	uint8_t size;
	uint8_t zero;
	uint16_t num_of_sectors;
	uint16_t dest_offset;
	uint16_t dest_segment;
	uint32_t lower_lba;
	uint32_t higher_lba;
} __attribute__((packed)) dap;


__attribute__((section(".bootloader32var"), aligned(16)))
dap rfd_dap = {0};

__attribute__((section(".bootloader32")))
void read_from_disk(uint16_t disk, uint16_t num_of_sectors, uint16_t dest_offset, uint16_t dest_segment, uint64_t lba) {
	rfd_dap.size = 16;
	rfd_dap.num_of_sectors = num_of_sectors;
	rfd_dap.dest_offset = dest_offset;
	rfd_dap.dest_segment = dest_segment;
	rfd_dap.lower_lba = lba & 0xffffffff;
	rfd_dap.higher_lba = (lba >> 32) & 0xffffffff;

	asm volatile("mov %0, %%si" : : "r"((uint16_t)&rfd_dap) : "si");
	asm volatile("mov %0, %%dl" : : "r"((uint8_t)disk) : "dl");
	asm volatile("mov $0x42, %%ah" : : : "ah");
	asm volatile("int $0x13");
	asm volatile("jc hcf");
}

__attribute__((section(".bootloader32"), noreturn, naked))
void bootloader_32(void) {
	/*
	eax contains memory_map_entries
	ebx contains video_mode_info
	ecx contains size_of_bootloader
	edx contains boot_disk
	*/
	uint32_t memory_map;
	uint32_t video_mode_info;
	uint32_t size_of_bootloader;
	uint32_t boot_disk;
	asm volatile("mov %%eax, %0" : "=r"(memory_map) : : );
	asm volatile("mov %%ebx, %0" : "=r"(video_mode_info) : : );
	asm volatile("mov %%ecx, %0" : "=r"(size_of_bootloader) : : );
	asm volatile("mov %%edx, %0" : "=r"(boot_disk) : : ); 
	/*
	crash if sizes dont match, it makes no sense to continue
	*/
  	if (sizeof(uint16_t) != 2 && sizeof(uint32_t) != 4 && sizeof(uint64_t) != 8) {
  		asm volatile("int $0xff");
  	}

  	uint64_t lba_of_elf = (512 + ((size_of_bootloader + 511) & (~511))) / 512;  
  	/*
  	we read the elf header into ram at 0x7c00
	we just override the boot sector, since we dont need it anymore
  	*/
  	read_from_disk(boot_disk, 1, 0x7c00, 0, lba_of_elf);
  	elf_header *elf = 0x7c00;

  	/*
	calculate the size of the program headers
	and the lba we need to load + offset from lba
  	*/
  	uint32_t size_of_program_headers = elf->phnum * elf->phentsize;
  	uint64_t lba_of_program_headers = lba_of_elf + (elf->phoff / 512);
  	uint32_t program_header_offset_from_lba = elf->phoff % 512;

  	/*
	TODO: 
		load the array of program headers into memory
		parse them and load the code of the ones with type == 1 into memory
			e.g. use offset in elf file ph->offset and ph->filesz to load them into memory
				the address at which is loaded doesnt matter
		enter protected mode
		copy the programs loaded from the program header to ph->vaddr - 0xFFFFFFFF80000000 + 0x100000
		set up a paging table -> map the address space from 1MB (0x100000) - 2GB1MB to 0xFFFFFFFF80000000 - 0xFFFFFFFFFFFFFFFF
		(maybe map physical memory) -> can also be done in kernel, so it doesnt really matter
		enter long mode
		jump to kernel -> elf->entry
  	*/

  	hcf();
}