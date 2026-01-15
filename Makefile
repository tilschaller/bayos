.PHONY: img
img:
	mkdir -p img
	$(MAKE) -C boot/bios install
	$(MAKE) -C kernel install
	truncate -s 65536 img/boot
	cat img/boot img/kernel > img/bayos-img
	truncate -s %512 img/bayos-img

.PHONY: run
run: img
	qemu-system-x86_64 -drive file=img/bayos-img,format=raw -monitor stdio -m 4G

.PHONY: clean
clean:
	rm -rf img
	$(MAKE) -C boot/bios clean
	$(MAKE) -C kernel clean
