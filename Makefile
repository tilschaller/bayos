.PHONY: bios-img
bios-img:
	# NOTE: i wont be maintaining bios boot as well as uefi
	# maybe it wont even work in subsequent versions
	# anyone is free to work on it
	mkdir -p img
	$(MAKE) -C boot/bios install
	$(MAKE) -C kernel install
	truncate -s 65536 img/boot
	cat img/boot img/kernel > img/bayos-img
	truncate -s %512 img/bayos-img

.PHONY: uefi-img
uefi-img:
	mkdir -p img
	$(MAKE) -C boot/uefi install
	$(MAKE) -C kernel install
	dd if=/dev/zero of=img/bayos-img bs=1k count=1440
	mformat -i img/bayos-img -f 1440 ::
	mmd -i img/bayos-img ::/EFI
	mmd -i img/bayos-img ::/EFI/BOOT
	mmd -i img/bayos-img ::/BOOT
	mcopy -i img/bayos-img img/boot.efi ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i img/bayos-img img/kernel ::/BOOT/KERNEL.ELF

.PHONY: bios-run
bios-run: bios-img
	qemu-system-x86_64 -drive file=img/bayos-img,format=raw -monitor stdio -m 4G

.PHONY: run
run: uefi-img
	cp /usr/share/ovmf/x64/OVMF.4m.fd img/ovmf.fd
	qemu-system-x86_64 -bios img/ovmf.fd -hda img/bayos-img -monitor stdio -m 4G

.PHONY: clean
clean:
	rm -rf img
	$(MAKE) -C boot/bios clean
	$(MAKE) -C boot/uefi distclean
	$(MAKE) -C kernel clean
