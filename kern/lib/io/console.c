
#include <console.h>

#define MAX_CSFRAMES 16
#define MAX_CELLS 160
#define MAX_LINES 50
#define CHAR_WIDTH 2

static char *const vga_buffer = (char *)0xB8000;

struct console_state {
	int line;
	int cell;
};

static struct console_state cs_frames[MAX_CSFRAMES];
static int frame_index;
static int enable_scrolling;

static struct console_state *current_csframe(void)
{
	return (cs_frames + frame_index);
}

void init_console(void)
{
	clear_console();

	int i;
	for (i = 0; i < MAX_CSFRAMES; ++i) {
		struct console_state *csf = (cs_frames + i);
		csf->line = 0;
		csf->cell = 0;
	}

	frame_index = 0;
	enable_scrolling = 1;
}

void acquire_console(void)
{

}

void clear_console(void)
{
	ssize_t i;
	for (i = 0; i < MAX_CELLS * MAX_LINES; i += 2) {
		vga_buffer[i] = ' ';
		vga_buffer[i + 1] = 0x00;
	}
}

void scroll_down(void)
{
	ssize_t i, j;
	for (i = 0, j = MAX_CELLS; i < MAX_CELLS * (MAX_LINES - 1); i += 2, j += 2) {
		vga_buffer[i] = vga_buffer[j];
		vga_buffer[i + 1] = 0x0F;
	}

	while (i < MAX_CELLS * MAX_LINES) {
		vga_buffer[i++] = ' ';
		vga_buffer[i++] = 0x00;
	}
}

void newline(void)
{
	struct console_state *ccsf = current_csframe();
	int line = ccsf->line;

	ccsf->cell = 0;

	line++;
	if (line == MAX_LINES) {
		if (enable_scrolling) {
			scroll_down();
		} else {
			line = 0;
		}
	}

	ccsf->line = line;
}

ssize_t write_console(void *buf, size_t count)
{
	const size_t max_unwritten = count;
	const char *bytes = buf;
	struct console_state *ccsf = current_csframe();

	int line = ccsf->line;
	int cell = ccsf->cell;

	while (count) {
		int index = line * MAX_CELLS + cell;
		int chr = *bytes++;

		switch (chr) {
		case '\n': line++;
		case '\r': cell = 0; break;
		default: {
			vga_buffer[index] = chr;
			vga_buffer[index + 1] = 0x0F;

			cell = cell + CHAR_WIDTH;
			if (cell == MAX_CELLS) {
				newline();
			}
		}
		}

		if (line == MAX_LINES) {
			if (enable_scrolling) {
				scroll_down();
			} else {
				line = 0;
			}
		}

		count--;
	}

	ccsf->line = line;
	ccsf->cell = cell;

	return (ssize_t)(max_unwritten - count);
}

void release_console(void)
{

}