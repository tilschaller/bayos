#include <uefi.h>

/*** ELF64 defines and structs ***/
#define ELFMAG      "\177ELF"
#define SELFMAG     4
#define EI_CLASS    4       /* File class byte index */
#define ELFCLASS64  2       /* 64-bit objects */
#define EI_DATA     5       /* Data encoding byte index */
#define ELFDATA2LSB 1       /* 2's complement, little endian */
#define ET_EXEC     2       /* Executable file */
#define PT_LOAD     1       /* Loadable program segment */
#ifdef __x86_64__
#define EM_MACH     62      /* AMD x86-64 architecture */
#endif
#ifdef __aarch64__
#define EM_MACH     183     /* ARM aarch64 architecture */
#endif

typedef struct
{
    uint8_t  e_ident[16];   /* Magic number and other info */
    uint16_t e_type;        /* Object file type */
    uint16_t e_machine;     /* Architecture */
    uint32_t e_version;     /* Object file version */
    uint64_t e_entry;       /* Entry point virtual address */
    uint64_t e_phoff;       /* Program header table file offset */
    uint64_t e_shoff;       /* Section header table file offset */
    uint32_t e_flags;       /* Processor-specific flags */
    uint16_t e_ehsize;      /* ELF header size in bytes */
    uint16_t e_phentsize;   /* Program header table entry size */
    uint16_t e_phnum;       /* Program header table entry count */
    uint16_t e_shentsize;   /* Section header table entry size */
    uint16_t e_shnum;       /* Section header table entry count */
    uint16_t e_shstrndx;    /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
    uint32_t p_type;        /* Segment type */
    uint32_t p_flags;       /* Segment flags */
    uint64_t p_offset;      /* Segment file offset */
    uint64_t p_vaddr;       /* Segment virtual address */
    uint64_t p_paddr;       /* Segment physical address */
    uint64_t p_filesz;      /* Segment size in file */
    uint64_t p_memsz;       /* Segment size in memory */
    uint64_t p_align;       /* Segment alignment */
} Elf64_Phdr;

int main(int argc, char **argv) {
	printf("Booting BAYOS\n");
	efi_status_t status;

	// load the kernel executabl
	FILE *file;
	char *buff;
	long int size;
	Elf64_Ehdr *elf;
	Elf64_Phdr *phdr;
	uintptr_t entry;
	int i;

	if ((f = fopen("\\BOOT\\KERNEL.ELF", "r"))) {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);
		buff = malloc(size + 1);
		if (!buff) {
			fprintf(stderr, "unable to allocate memory\n");
			return 1;
		}
		fread(buff, size, 1, f);
		fclose(f);
	} else {
		fprintf(stderr, "Unable to open file\n");
		return 0;
	}

	elf = (Elf64_Ehdr *)buff;
	if (!memcmp(elf->e_ident, ELFMAG, SELFMAG) && 
	    elf->e_ident[EI_CLASS] == ELFCLASS64 && 
	    elf->e_ident[EI_DATA] == ELFDATA2LSB &&
	    elf->e_type == ET_EXEC && 
	    elf->machine == EM_MACH &&
	    elf->e_phnum > 0) {
		for (phdr = (Elf64_Phdr *)(buff + elf->phoff), i = 0; i < elf->e_phnum; i++, phdr = (Elf64_Phdr *)((uint8_t*)phdr + elf->e_phentsize)) {
			if (phdr->p_type == PT_LOAD) {
				printf("ELF segment %p %d bytes (bss %d bytes)\n", phdr->p_vaddr, phdr->p_filesz, phdr->p_memsz - phdr->p_filesz);


			
	// get memory map very last

	efi_memory_descriptor_t *memory_map = NULL, *mement;
	uintn_t memory_map_size = 0, map_key = 0, desc_size = 0;
	const char *types[] = {
		"EfiReservedMemoryType",
        	"EfiLoaderCode",
        	"EfiLoaderData",
        	"EfiBootServicesCode",
        	"EfiBootServicesData",
        	"EfiRuntimeServicesCode",
        	"EfiRuntimeServicesData",
        	"EfiConventionalMemory",
        	"EfiUnusableMemory",
        	"EfiACPIReclaimMemory",
        	"EfiACPIMemoryNVS",
        	"EfiMemoryMappedIO",
        	"EfiMemoryMappedIOPortSpace",
        	"EfiPalCode"
	};

	status = BS->GetMemoryMap(&memory_map_size, NULL, &map_key, &desc_size, NULL);
	if (status != EFI_BUFFER_TOO_SMALL || !memory_map_size) goto err;

	memory_map_size += 4 + desc_size;
	memory_map = (efi_memory_descriptor_t*)malloc(memory_map_size);
	if (!memory_map) {
		fprintf(stderr, "unable to allocate memory\n");
		return 1;
	}

	status = BS->GetMemoryMap(&memory_map_size, memory_map, &map_key, &desc_size, NULL);
	if (EFI_ERROR(status)) {
err:
		fprintf(stderr, "Unable to get memory map\n");
		return 0;
	}

	
	printf("Address              Size Type\n");
	for(mement = memory_map; (uint8_t*)mement < (uint8_t*)memory_map + memory_map_size;
	    mement = NextMemoryDescriptor(mement, desc_size)) {
		printf("%016x %8d %02x %s\n", mement->PhysicalStart, mement->NumberOfPages, mement->Type, types[mement->Type]);
	}

	while (1) {}
	return 0;
}
