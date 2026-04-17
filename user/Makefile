AS = nasm
LD = ld

init: init.asm
	nasm -f elf64 -o init.o init.asm
	ld -o init init.o

.PHONY: clean
clean:
	rm -rf init.o init
