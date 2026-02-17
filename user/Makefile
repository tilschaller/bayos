AS = nasm
LD = ld

test: test.asm
	nasm -f elf64 -o test.o test.asm
	ld -o test test.o

.PHONY: clean
clean:
	rm -rf test.o test

.PHONY: install
install: test
	cp test ../iso
