.PHONY: img
img:
	mkdir -p img
	$(MAKE) -C boot install
	$(MAKE) -C kernel install
	truncate -s %512 img/boot
	cat img/boot img/kernel > img/bayos-img

.PHONY: run
run: img
	qemu-system-x86_64 -drive file=img/bayos-img,format=raw

.PHONY: clean
clean:
	rm -rf img
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
