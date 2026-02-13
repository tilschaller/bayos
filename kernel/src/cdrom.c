#include <cdrom.h>
#include <types/types.h>
#include <ports.h>
#include <lock.h>

#define DATA 0
#define ERROR_R 1
#define SECTOR_COUNT 2
#define LBA_LOW 3
#define LBA_MID 4
#define LBA_HIGH 5
#define DRIVE_SELECT 6
#define COMMAND_REGISTER 7

#define CONTROL 0x206
#define ALTERNATE_STATUS 0

// wait 400 ns
static void ata_io_wait(const uint8_t p) {
	read_port_u8(p + CONTROL + ALTERNATE_STATUS);
	read_port_u8(p + CONTROL + ALTERNATE_STATUS);
	read_port_u8(p + CONTROL + ALTERNATE_STATUS);
	read_port_u8(p + CONTROL + ALTERNATE_STATUS);
}

static spinlock_t lock;

// read sectors starting from lba to buffer
int read_cdrom(uint16_t port, bool slave, uint32_t lba, uint32_t sectors, uint16_t *buffer) {
	acquire(&lock);
	int ret;
	volatile uint8_t read_cmd[12] = { 0xA8, 0,
	                                  (lba >> 0x18) & 0xff, (lba >> 0x10) & 0xff, (lba >> 0x8) & 0xff, (lba >> 0x0) & 0xff,
	                                  (sectors >> 0x18) & 0xff, (sectors >> 0x10) & 0xff, (sectors >> 0x8) & 0xff, (sectors >> 0x0) & 0xff,
	                                  0, 0
	                                };
	write_port_u8(port + DRIVE_SELECT, 0xa0 & (slave << 4));
	ata_io_wait(port);
	write_port_u8(port + ERROR_R, 0x0);
	write_port_u8(port + LBA_MID, 2048 & 0xff);
	write_port_u8(port + LBA_HIGH, 2048 >> 8);
	write_port_u8(port + COMMAND_REGISTER, 0xa0);
	ata_io_wait(port);

	for (;;) {
		uint8_t status = read_port_u8(port + COMMAND_REGISTER);
		if ((status & 1) == 1) {
			ret = 1;
			goto end;
		}
		if (!(status & 0x80) && (status & 0x8))
			break;
		ata_io_wait(port);
	}

	outsw(port + DATA, (uint16_t*) read_cmd, 6);

	for (uint32_t i = 0; i < sectors; i++) {
		for (;;) {
			uint8_t status = read_port_u8(port + COMMAND_REGISTER);
			if (status & 1) {
				ret = 1;
				goto end;
			}
			if (!(status & 0x80) && (status & 0x8))
				break;
		}

		int size = read_port_u8(port + LBA_HIGH) << 8 | read_port_u8(port + LBA_MID);

		insw(port + DATA, (uint16_t *)((uint8_t*)buffer + i * 0x800), size / 2);
	}

	ret = 0;
end:
	release(&lock);
	return ret;
}
