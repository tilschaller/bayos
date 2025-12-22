#include <framebuffer.h>
#include <string.h>
#include <types/types.h>
#include <memory.h>
/*
this is automatically set to the first framebuffer created,
but can be overwritten manually
all calls to putchar use this framebuffer
TODO:
	we need a lock around this
*/
framebuffer stdout = {0};

/*
	symbols are provided by font.o asset
	TODO: search a nicer font, because this one is ugly
*/
extern char _binary_zap_vga16_psf_start;
extern char _binary_zap_vga16_psf_end;

#define PSF_FONT_MAGIC 0x0436
typedef struct {
    uint16_t magic;
    uint8_t fontMode;
    uint8_t characterSize;
} PSF1_Header;

/*
	NOTE: we assume a 32bit rgb mode framebuffer,
	which is not guaranteed by the bootloader
*/

#define LINE_SPACING 2
#define LETTER_SPACING 0
#define BORDER_PADDING 1

#define CHAR_RASTER_HEIGHT 16
#define CHAR_RASTER_WIDTH 8
#define BACKUP_CHAR 32

static void carriage_return() {
	stdout.x_pos = BORDER_PADDING;
}

static void newline() {
	stdout.y_pos += CHAR_RASTER_HEIGHT + LINE_SPACING;
	carriage_return();
}


static uint64_t width() {
	return (uint64_t)stdout.info->height;
}

static uint64_t height() {
	return (uint64_t)stdout.info->height;
}

static void clear() {
	uint64_t size = (uint64_t)(stdout.info->bpp / 8) * width() * height();
	stdout.x_pos = BORDER_PADDING;
	stdout.y_pos = BORDER_PADDING;
	memset(stdout.fb, 0, size);
}

static void write_pixel(uint64_t x, uint64_t y, uint32_t color) {
	uint32_t* fb = stdout.fb;
	fb[x + (y * (stdout.info->pitch / 4))] = color;
}

static void write_char(unsigned char c, uint32_t color) {
	int bytes_per_glyph = (CHAR_RASTER_WIDTH * CHAR_RASTER_HEIGHT) / 8;

	uint8_t *glyph = (uint8_t*)&_binary_zap_vga16_psf_start + 
		sizeof(PSF1_Header) + 
		c * bytes_per_glyph;

	for (int y = 0; y < CHAR_RASTER_HEIGHT; y++) {
		uint8_t pixel_row = glyph[y];
		for (int x = 0; x < CHAR_RASTER_WIDTH; x++) {
			if (pixel_row & (1 << (7 - x))) {
				write_pixel(stdout.x_pos + x, stdout.y_pos + y, color);
			}
		}
	}

	stdout.x_pos += CHAR_RASTER_WIDTH + LETTER_SPACING;
}

int putchar(int c) {
	switch (c) {
	case '\n':
		newline();
		break;
	case '\r':
		carriage_return();
		break;
	default:
		uint64_t new_xpos = stdout.x_pos + CHAR_RASTER_WIDTH;
		if (new_xpos >= width()) {
			newline();
		}
		uint64_t new_ypos = stdout.y_pos + CHAR_RASTER_HEIGHT + BORDER_PADDING;
		if (new_ypos >= height()) {
			clear();
		}
		write_char((char)c, 0xffffff);
	}

	return 0;
}

void fb_init(video_mode_info *info) {
	stdout.info = info;
	stdout.x_pos = 0;
	stdout.y_pos = 0;
	stdout.fb = phys_to_virt((uint64_t)info->phys_address);
}
