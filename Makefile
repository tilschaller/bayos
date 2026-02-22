.PHONY: bayos.iso
bayos.iso: limine/limine
	rm -rf iso
	mkdir -p iso/boot
	$(MAKE) -C kernel install
	mkdir -p tar
	$(MAKE) -C user install
	(cd iso/boot; tar --transform='s|^tar/||' -cf initrd.tar ../../tar)
	rm -rf tar
	mkdir -p iso/boot/limine
	cp -v limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso/boot/limine/
	mkdir -p iso/EFI/BOOT
	cp -v limine/BOOTX64.EFI iso/EFI/BOOT/
	cp -v limine/BOOTIA32.EFI iso/EFI/BOOT/
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso -o bayos.iso
	./limine/limine bios-install bayos.iso
	rm -rf iso

.PHONY: run
run: bayos.iso
	qemu-system-x86_64 -cdrom bayos.iso -monitor stdio -m 4G -boot d -M q35

.PHONY: clean
clean:
	rm -rf iso/
	rm -f bayos.iso
	rm -rf limine/
	$(MAKE) -C kernel clean
	$(MAKE) -C user clean

limine/limine:
	rm -rf limine/
	git clone https://codeberg.org/Limine/Limine.git limine --branch=v10.x-binary --depth=1
	$(MAKE) -C limine 
