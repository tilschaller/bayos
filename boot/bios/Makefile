LD = ld
AS = nasm

SRCS = \
	bootloader.asm

OBJS = $(patsubst %.asm,%.o,$(SRCS))

boot: $(OBJS) boot.ld Makefile
	$(LD) -m elf_i386 -o $@ $(OBJS) -T boot.ld

%.o: %.asm Makefile
	$(AS) -f elf32 $< -o $@

.PHONY: clean
clean: 
	rm -f boot $(OBJS)

.PHONY: install
install: boot
	cp boot ../../iso/boot.bin
