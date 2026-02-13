AS = nasm
LD = ld

shell: shell.asm
	nasm -f elf64 -o shell.o shell.asm
	ld -o shell shell.o

.PHONY: clean
clean:
	rm -rf shell.o shell

.PHONY: install
install: shell
	cp shell ../iso
