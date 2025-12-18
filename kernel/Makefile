LD = ld
AS = nasm
CC = gcc

CFLAGS = \
	-m64 \
	-fno-pie \
	-nostdlib \
	-Wall -Wextra \
	-fno-builtin \
	-fno-exceptions \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables \
	-fno-ident

ASM_SRCS =
C_SRCS = \
	main.c

ASM_OBJS = $(patsubst %.asm,%.o,$(ASM_SRCS))
C_OBJS = $(patsubst %.c,%.o,$(C_SRCS))
OBJS = $(ASM_OBJS) $(C_OBJS)

kernel: $(OBJS) kernel.ld Makefile
	$(LD) -o $@ $(OBJS) -T kernel.ld

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
