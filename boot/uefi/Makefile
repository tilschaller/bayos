TARGET = boot.efi

USE_GCC = 1

include uefi/Makefile

.PHONY: install
install: boot.efi
	cp boot.efi ../../img
