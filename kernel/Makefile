LD = ld
AS = nasm
CC = gcc

CFLAGS = \
	-m64 \
	-Iinclude \
	-nostdlib \
	-Wall -Wextra \
	-fno-pie \
	-fno-builtin \
	-fno-exceptions \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-ident \
	-fno-stack-protector \
	-mno-red-zone \
	-mcmodel=kernel \
	-MMD \
	-mgeneral-regs-only \
	-O2

ASM_SRCS = \
	src/exception_handler.asm

C_SRCS = \
	src/main.c \
	src/string.c \
	src/framebuffer.c \
	src/fail.c \
	src/memory.c \
	src/stdio/printf.c \
	src/stdio/puts.c \
	src/gdt.c \
	src/interrupts.c \
	src/stdio/num_sys.c \
	src/alloc.c \
	src/acpi.c \
	src/ports.c \
	src/keyboard.c \
	src/scheduler.c \
	src/lock.c \
	src/cdrom.c \
	src/elf.c

ASM_OBJS = $(patsubst %.asm,%.o,$(ASM_SRCS))
C_OBJS = $(patsubst %.c,%.o,$(C_SRCS))
OBJS = $(ASM_OBJS) $(C_OBJS)

-include $(C_OBJS:.o=.d)

kernel: $(OBJS) kernel.ld Makefile assets/font.o
	$(LD) -o $@ $(OBJS) assets/font.o -T kernel.ld

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm Makefile
	$(AS) -f elf64 $< -o $@

.PHONY: clean
clean:
	rm -f kernel $(OBJS) $(C_OBJS:.o=.d)

.PHONY: install
install: kernel
	cp kernel ../iso
