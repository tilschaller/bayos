#include <tar.h>
#include <alloc.h>
#include <fail.h>

static unsigned int get_size(const char *in) {
	unsigned int size = 0;
	unsigned int j;
	unsigned int count = 1;

	for (j = 11; j > 0; j--, count *= 8)
		size += ((in[j -1] - '0') * count);

	return size;
}

static unsigned get_number_of_entries(uintptr_t address) {
	unsigned int i;

	for (i = 0; ; i++) {
		tar_header *header = (tar_header *)address;

		if (header->filename[i] == '\0')
			break;

		unsigned int size = get_size(header->size);

		address += ((size / 512) + 1) * 512;

		if (size % 512)
			address += 512;
	}

	return i;
}

tar_archive *tar_create(uintptr_t tar) {
	tar_archive *archive = alloc(sizeof(tar_archive));
	if (!archive)
		return NULL;

	archive->entry_count = get_number_of_entries(tar);

	archive->headers = alloc(sizeof(tar_header*) * archive->entry_count);
	if (!archive->headers) {
		free(archive);
		return NULL;
	}

	for (int i = 0; ; i++) {
		tar_header *header = (tar_header *)tar;

		if (header->filename[i] == '\0')
			break;

		unsigned int size = get_size(header->size);

		archive->headers[i] = header;

		tar += ((size / 512) + 1) * 512;

		if (size % 512)
			tar += 512;
	}

	return archive;
}

void tar_delete(tar_archive* tar) {
	free(tar->headers);
	free(tar);
}
