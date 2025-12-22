LD = ld
AS = nasm
CC = gcc

CFLAGS = \
	-m64 \
	-Iinclude \
	-nostdlib \
	-Wall -Wextra \
	-fno-exceptions \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-ident

ASM_SRCS =

C_SRCS = \
	src/main.c \
	src/string.c \
	src/framebuffer.c \
	src/fail.c \
	src/memory.c \
	src/stdio/printf.c

ASM_OBJS = $(patsubst %.asm,%.o,$(ASM_SRCS))
C_OBJS = $(patsubst %.c,%.o,$(C_SRCS))
OBJS = $(ASM_OBJS) $(C_OBJS)

kernel: $(OBJS) kernel.ld Makefile assets/font.o
	$(LD) -o $@ $(OBJS) assets/font.o -T kernel.ld

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm Makefile
	$(AS) -f elf64 $< -o $@

.PHONY: clean
clean:
	rm -f kernel $(OBJS)

.PHONY: install
install: kernel
	cp kernel ../img
