#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <console.h>
#include <algo.h>

#define FORMAT_INDICATOR_CHR ('%')

enum format_data_type {
	FMTDATA_INT,
	FMTDATA_CHR,
	FMTDATA_STR,
	FMTDATA_FLT,
	FMTDATA_ESC
};

enum format_alignment {
	FMTALGN_LEFT,
	FMTALGN_RIGHT
};

struct format_arg_info {
	
	/**
	 *  Generic members, usable in any context.
	 **/
	int width; /* Minimum width of the outputted format. */
	int precision; /* Defined per outputted data type. */
	int pad; /* Character to pad leftover width with. */
	bool alternate_form; /* # flag. */
	enum format_alignment align; /* Pad to the left or right. */
	enum format_data_type data; /* Type of data to be written. */
	int size; /* Defined per outputted data type. */

	/**
	 *  Type-specific members, use DATA to determine
	 *  which struct is appropiate.
	 **/
	union {
		
		/* INT data */
		struct {
			/* int size; %<int>, %l<int>, %ll<int> */
			int base; /* %i, %o, %x */
			int sign_pad; /* Character to pad the sign region with, if any. */
			bool szt; /* Print SIZE_T or SSIZE_T? */
			bool ptd; /* Print PTRDIFF_T? */
			bool write_signed; /* %i, %d vs %u, %z */	
			bool upper_digits; /* abcdef vs ABCDEF */
		};

		/* FLOAT data */
		struct {
			/* int size; %<float>, %L<float> */
			bool exponential; /* Print in exponential format? */
			bool hexponential; /* Print in hex exponential format? */
			bool upperconst; /* Print constants in all caps? */
		};

	};
};

static void fmt_defaults(struct format_arg_info *out)
{
	out->width = 1;
	/* This is NOT the default precision, but simply
	   a placeholder value to indicate that it has
	   not been set. */
	out->precision = -1;
	out->pad = ' ';
	out->alternate_form = false;
	out->align = FMTALGN_LEFT;
	out->data = FMTDATA_INT;
	out->size = 0;
}

static char *read_format(const char *format, struct format_arg_info *pfarg)
{
	const char *flagspecs = "#0- +";
	const char *sizespecs = "lhztjL";
	const char *convspecs = "diouxXeEfFgGaAcspn%";

	/* Skip xprintf() format delimeter. */
	format++;
	int chr = *format++;

	while (chr && strchr(flagspecs, chr)) {
		switch (chr) {
			case '#': pfarg->alternate_form = true; break;
			case '0': {
				if (pfarg->align != FMTALGN_RIGHT) {
					pfarg->pad = '0';
				}
			}
			case '-': {
				pfarg->pad = ' ';
				pfarg->align = FMTALGN_RIGHT;
			} break;
			case ' ': {
				if (pfarg->sign_pad != '+') {
					pfarg->sign_pad = ' ';
				}
			} break;
			case '+': pfarg->sign_pad = '+'; break;
		}	

		chr = *format++;
	}

	if (strchr(sizespecs, chr)) {
		switch (chr) {
			case 'l': {
				int size = 8;
				chr = *format;
				if (chr == 'l') {
					size = 16;
					format++;
				}
				pfarg->size = size;
			} break;
			case 'h': {
				int size = 2;
				chr = *format;
				if (chr == 'h') {
					size = 1;
					format++;
				}
				pfarg->size = size;
			} break;
			case 'z': {
				pfarg->size = sizeof(size_t);
				pfarg->szt = true;
			} break;
			case 't': {
				pfarg->size = sizeof(ptrdiff_t);
				pfarg->ptd = true;
			} break;
			case 'j': {} break;
			case 'L': pfarg->size = sizeof(long double); break;
		}

		chr = *format++;
	}

	pfarg->upper_digits = false;
	pfarg->base = 10;

	switch (chr) {
		case 'X': pfarg->upper_digits = true;
		case 'x': pfarg->base = 16;
		case 'o': {
			if (chr == 'o') {
				pfarg->base = 8;
			}
		}
		case 'u': {
			pfarg->data = FMTDATA_INT;
			pfarg->write_signed = false;
			if (pfarg->precision == -1) {
				pfarg->precision = 1;
			}

			if (pfarg->size == 0) {
				pfarg->size = 4;
			}
		} break;
		case 'i': {
			pfarg->data = FMTDATA_INT;
			pfarg->write_signed = true;
			pfarg->base = 10;
			if (pfarg->precision == -1) {
				pfarg->precision = 1;
			}
			if (pfarg->size == 0) {
				pfarg->size = 4;
			}
		} break;
		case 'c': {
			pfarg->data = FMTDATA_CHR;
		} break;
		case '%': {
			pfarg->data = FMTDATA_ESC;
		}
	}

	return (char *)format;
}

static char *write_int(char *dest, const struct format_arg_info *pfarg, va_list argv)
{
	const char *digits[] = { "0123456789abcdef", "0123456789ABCDEF" };

	bool write_signed = pfarg->write_signed;
	bool upper_digits = pfarg->upper_digits;
	int base = pfarg->base;

	long long unsigned value;
	bool negative;

	if (pfarg->szt) {
		if (write_signed) {
			value = va_arg(argv, ssize_t);
		} else {
			value = va_arg(argv, size_t);
		}
	} else if (pfarg->ptd) {
		value = va_arg(argv, ptrdiff_t);
	} else {
		switch (pfarg->size) {
		case 1: value = va_arg(argv, int) & 0xFF;   break;
		case 2: value = va_arg(argv, int) & 0xFFFF; break;
		case 4: value = va_arg(argv, int);          break;
		case 8: value = va_arg(argv, long);			break;
		case 16: value = va_arg(argv, long long); 	break;
		}
	}

	negative = (write_signed && isnegative(value, pfarg->size));
	char revint[256];
	char *prevint = revint;

	if (value) {
		while (value > 0 && prevint != (revint + 256)) {
			int digit = (int)(value % base);
			*prevint++ = digits[upper_digits][digit];
			value = value / base;
		}
	} else {
		*prevint++ = '0';
	}

	int nwrote = (int)(prevint - revint);
	int npadchrs = max(0, pfarg->width - nwrote);
	int pad = pfarg->pad;
	
	if (pfarg->align == FMTALGN_LEFT) {
		while (npadchrs > 1) {
			*dest++ = pad;
			npadchrs--;
		}

		if (npadchrs) {
			switch (pfarg->sign_pad) {
				case ' ': *dest++ = (negative) ? '-' : ' '; break;
				case '+': *dest++ = (negative) ? '-' : '+'; break;
				default: {
					if (negative) {
						*dest++ = '-';
					} else {
						*dest++ = pad;
					}
				} break;
			}
		}

		prevint--;

		while (nwrote--) {
			int chr = *prevint--;
			*dest++ = chr;
		}
	}

	return dest;
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
	case FMTDATA_ESC: *dest++ = '%'; break;
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