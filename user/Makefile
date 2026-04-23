AS = nasm
LD = ld

init: init.asm
	nasm -f elf64 -o init.o init.asm
	ld.lld -o init init.o -m elf_x86_64

.PHONY: clean
clean:
	rm -f init.o init
