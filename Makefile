.PHONY: img
img:
	mkdir -p img
	$(MAKE) -C boot install
	$(MAKE) -C kernel install
	cp img/boot img/bayos-img
	truncate -s %512 img/bayos-img
	cat img/bayos-img img/kernel > img/bayos-img

.PHONY: clean
clean:
	rm -rf img
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
