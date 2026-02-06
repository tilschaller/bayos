#ifndef _CDROM_H
#define _CDROM_H

#include <types/types.h>

#define CDROM_BASE_PORT 0x170

int read_cdrom(uint16_t port, bool slave, uint32_t lba, uint32_t sectors, uint16_t *buffer);

#endif // _CDROM_H
