#ifndef _TAR_H
#define _TAR_H

#include <types/types.h>

typedef struct {
	char filename[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char typeflag[1];
	char padding[355];
	uint8_t file[];
} tar_header;

typedef struct {
	int entry_count;
	tar_header **headers;
} tar_archive;

tar_archive *tar_create(uintptr_t tar);
void tar_delete(tar_archive* tar);

#endif // _TAR_H
