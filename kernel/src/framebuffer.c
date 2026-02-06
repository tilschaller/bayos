#include <framebuffer.h>
#include <string.h>
#include <types/types.h>
#include <memory.h>
#include <lock.h>
/*
this is automatically set to the first framebuffer created,
but can be overwritten manually
all calls to putchar use this framebuffer
*/
spinlock_t lock;
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

#define TAB_SIZE 8

int32_t putchar(int32_t c);

static uint64_t width() {
	return (uint64_t)stdout.info->width;
}

static uint64_t height() {
	return (uint64_t)stdout.info->height;
}

static void carriage_return() {
	stdout.x_pos = BORDER_PADDING;
}

static void newline() {
	stdout.y_pos += CHAR_RASTER_HEIGHT + LINE_SPACING;
	carriage_return();
}

uint64_t get_new_htab(uint64_t cur_x) {
	/*Note: a next htab cannot be 0, so 0 will be handled as err-code*/
	uint64_t htab = BORDER_PADDING;
	while(cur_x >= htab) {
		htab += TAB_SIZE * CHAR_RASTER_WIDTH;
	}

	return htab;
}

static void htab() {
	/* if current position is between the last tab-stop and the end of screen
	 * the overflow will be handled as a newline()*/
	if(stdout.x_pos >= (width() - (TAB_SIZE * CHAR_RASTER_WIDTH))) {
		newline();
	} else {
		stdout.x_pos = get_new_htab(stdout.x_pos);
	}
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

static void backspace() {
	/* if (for whatever reason) the current cursor position is smaller then a
	 * single-character width, it should be set to the border. Also if the 
	 * current position is at the 0, it is undefined in C99 standard, so we will
	 * handle it to set x_pos at the border*/
	if(stdout.x_pos < (CHAR_RASTER_WIDTH + BORDER_PADDING)) {
		stdout.x_pos = BORDER_PADDING;
	} else {
		stdout.x_pos -= CHAR_RASTER_WIDTH;
	}

	for(int i = 0; i < CHAR_RASTER_HEIGHT; i++) {
			for(int j = 0; j < CHAR_RASTER_WIDTH; j++) {
				write_pixel(stdout.x_pos + j, stdout.y_pos + i, 0x000000);
			}
	}
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

int32_t putchar(int32_t c) {
	acquire(&lock);

	switch (c) {
	case '\n':
		newline();
		break;
	case '\r':
		carriage_return();
		break;
	case '\b':
		backspace();
		break;
	case '\t':
		htab();
		break;
	default:
		(void)0;
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

	release(&lock);

	return 0;
}

void fb_init(video_mode_info *info) {
	release(&lock);
	stdout.info = info;
	stdout.x_pos = BORDER_PADDING;
	stdout.y_pos = BORDER_PADDING;
	stdout.fb = phys_to_virt((uint64_t)info->phys_address);
	memset(stdout.fb, 0, info->width * info->height / (info->bpp / 8));
}
