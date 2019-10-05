#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <console.h>

#define FORMAT_INDICATOR_CHR ('%')

enum format_data_type {
	FMTDATA_INT,
	FMTDATA_CHR,
	FMTDATA_STR,
	FMTDATA_FLT,
};

struct format_arg_info {
	
	/**
	 *  Generic members, usable in any context.
	 **/
	int width; /* Minimum width of the outputted format. */
	int pad; /* Character to pad leftover width with. */
	enum format_data_type data; /* Type of data to be written. */

	/**
	 *  Type-specific members, use DATA to determine
	 *  which struct is appropiate.
	 **/
	union {
		
		/* INT data */
		struct {
			int size; /* %<int>, %l<int>, %ll<int> */
			bool write_signed; /* %i, %d vs %u, %z */	
			int base; /* %i, %o, %x */
		};

		/* FLOAT data */
		struct {
			bool exponential; /* Print in exponential format? */
			bool hexponential; /* Print in hex exponential format? */
		};

	};
};

static void fmt_defaults(struct format_arg_info *out)
{
	out->width = 1;
	out->pad = ' ';
	out->data = FMTDATA_INT;
}

static char *read_format(const char *format, struct format_arg_info *pfarg)
{
	// skip delimeter
	format++;


	int chr = *format++;

	switch (chr) {
		case 'x': {
			pfarg->data = FMTDATA_INT;

			pfarg->size = sizeof(int);
			pfarg->write_signed = false;
			pfarg->base = 16;
		} break;
		case 'i': {
			pfarg->data = FMTDATA_INT;

			pfarg->size = sizeof(int);
			pfarg->write_signed = true;
			pfarg->base = 10;
		} break;
		case 'c': {
			pfarg->data = FMTDATA_CHR;
		} break;
	}

	return (char *)format;
}

static char *write_int(char *dest, const struct format_arg_info *pfarg, va_list argv)
{

}

static char *write_format(char *dest, const struct format_arg_info *pfarg, va_list argv) 
{
	switch (pfarg->data) {
	case FMTDATA_INT: dest = write_int(dest, pfarg, argv); break; 
	case FMTDATA_CHR: {
		int chr = va_arg(argv, int);
		*dest++ = chr; 
	} break;
	case FMTDATA_STR: break; 
	case FMTDATA_FLT: break;
	}

	return dest;
}

int vsprintf(char *dest, const char *format, va_list argv)
{
	struct format_arg_info farg;
	int argc;
	const char *fmt_start, *fmt_end;

	argc = 0;
	fmt_start = format;
	fmt_end = strchr(format, FORMAT_INDICATOR_CHR);

	while (fmt_end) {
		size_t off = (size_t)(fmt_end - fmt_start);
		memcpy(dest, fmt_start, off);
		dest = dest + off;

		fmt_defaults(&farg);
		fmt_start = read_format(fmt_end, &farg);
		dest = write_format(dest, &farg, argv);	
		argc++;

		fmt_end = strchr(fmt_start, FORMAT_INDICATOR_CHR);
	}

	size_t end = strlen(fmt_start);

	memcpy(dest, fmt_start, end);
	dest[end] = 0;
	return argc;
}

#define PRINTF_GBUFSZ (0x100)

int sprintf(char *dest, const char *format, ...)
{
	int argc;
	va_list argv;

	va_start(argv, format);
	argc = vsprintf(dest, format, argv);
	va_end(argv);

	return argc;
}

static char gbuf[PRINTF_GBUFSZ];

int printf(const char *format, ...) 
{
	int argc;
	va_list argv;

	va_start(argv, format);
	argc = vsprintf(gbuf, format, argv);
	va_end(argv);

	write_console(gbuf, strlen(gbuf));

	return argc;
}