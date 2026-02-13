.PHONY: iso
iso:
	mkdir -p iso
	$(MAKE) -C boot/bios install
	$(MAKE) -C kernel install
	$(MAKE) -C user install
	mkisofs \
		-b boot.bin \
		-no-emul-boot \
		-boot-info-table \
		-o bayos.iso \
		iso/

.PHONY: run
run: iso
	qemu-system-x86_64 -cdrom bayos.iso -monitor stdio -m 4G

.PHONY: clean
clean:
	rm -rf iso/
	rm -f bayos.iso
	$(MAKE) -C boot/bios clean
	$(MAKE) -C kernel clean
	$(MAKE) -C user clean
