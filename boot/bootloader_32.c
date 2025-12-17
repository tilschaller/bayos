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

__attribute__((section(".bootloader32"), noreturn))
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

__attribute__((section(".bootloader32"), noreturn))
void parse_elf(uint32_t memory_map, uint32_t video_mode, uint32_t size_of_bootloader) {
	(void)memory_map;
	(void)video_mode;
	(void)size_of_bootloader;

	/*
	crash if sizes dont match, it makes no sense to continue
	*/
  	if (sizeof(uint16_t) != 2 && sizeof(uint32_t) != 4 && sizeof(uint64_t) != 8) {
  		asm volatile("int $0xff");
  	}

  	/*
  	TODO: 
  		read the program headers of kernel into memory
  			preferably at address 1MiB (0x100000)
  			using https://www.ctyme.com/intr/rb-1527.htm
  		enable nmis
  		set up paging for kernel
  		enter long mode
  		jump to kernel and pass memory map and video mode
  	*/ 

  	hcf();
}

uint32_t read_disk(uint32_t lba, uint32_t addr) {
	(void)lba;
	(void)addr;

	return 0;
}