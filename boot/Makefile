LD = ld
AS = nasm
CC = gcc

CFLAGS = \
	-m16 \
	-ffreestanding \
	-fno-pie \
	-nostdlib \
	-nostartfiles \
	-Wall -Wextra \
	-fno-builtin \
	-fno-exceptions \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-ident

ASM_SRCS = \
	boot_sector.asm \
	bootloader_16.asm

C_SRCS = \
	bootloader_32.c

ASM_OBJS = $(patsubst %.asm,%.o,$(ASM_SRCS))
C_OBJS = $(patsubst %.c,%.o,$(C_SRCS))
OBJS = $(ASM_OBJS) $(C_OBJS)

boot: $(OBJS) boot.ld Makefile
	$(LD) -m elf_i386 -o $@ $(OBJS) -T boot.ld

%.o: %.asm Makefile
	$(AS) -f elf32 $< -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@ 

.PHONY: test
test: boot
	qemu-system-x86_64 -drive file=$<,format=raw

.PHONY: clean
clean: 
	rm -f boot $(OBJS)

.PHONY: install
install: boot
	mv boot ../img
