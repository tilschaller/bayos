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

C_16_FLAGS = $(CFLAGS) -m16
C_32_FLAGS = $(CFLAGS) -m32

ASM_SRCS = \
	boot_sector.asm \
	bootloader_16.asm

C_16_SRCS = \
	bootloader_16_2.c16

C_32_SRCS = \
	bootloader_32.c32

ASM_OBJS = $(patsubst %.asm,%.o,$(ASM_SRCS))
C_16_OBJS = $(patsubst %.c16,%.o,$(C_16_SRCS))
C_32_OBJS = $(patsubst %.c32,%.o,$(C_32_SRCS))
OBJS = $(ASM_OBJS) $(C_16_OBJS) $(C_32_OBJS)

boot: $(OBJS) boot.ld Makefile
	$(LD) -m elf_i386 -o $@ $(OBJS) -T boot.ld

%.o: %.asm Makefile
	$(AS) -f elf32 $< -o $@

%.o: %.c16 Makefile
	$(CC) $(C_16_FLAGS) -c -x c $< -o $@

%.o: %.c32 Makefile
	$(CC) $(C_32_FLAGS) -c -x c $< -o $@

.PHONY: test
test: boot
	qemu-system-x86_64 -drive file=$<,format=raw

.PHONY: clean
clean: 
	rm -f boot $(OBJS)

.PHONY: install
install: boot
	mv boot ../img
