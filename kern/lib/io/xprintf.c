
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <console.h>
#include <stdbool.h>
#include <algo.h>

enum format_flag {
	FFLG_FLOAT,
	FFLG_STR,
	FFLG_CHR,
	FFLG_ESCAPE,
	FFLG_PTR,
	FFLG_INT,
	FFLG_SIZET
};

struct format_str_info {
	enum format_flag flg;
	int width;
	int pad;
	bool upper;

	// int writing
	bool write_signed;
	int size;
	int base;

	// float writing
	bool exponential;
};

static size_t get_format_info(const char *fmt, struct format_str_info *info)
{
	// skip useless garbage
	int chr = *++fmt;

	if (isdigit(chr)) {
		if (chr == '0') {
			info->pad = '0';
			fmt++;
		}

		
	}
}

static char *write_int(char *dest, const struct format_str_info *info, unsigned long long value)
{
	static const char digits[] = "0123456789abcdef";
	
	int transform = info->upper ? 32 : 0;
	char rev_int[sizeof(unsigned long long) * 8];

	int i, base;
	base = info->base;

	for (i = 0; i < sizeof(unsigned long long) * 8 && value > 0; ++i, value = value / base) {
		rev_int[i] = digits[value % base];
	}

	int npadchrs = min(0, info->width - i);
	int pad = info->pad;

	while (npadchrs--) {
		*dest++ = (char)pad;
	} 

	for (i = i - 1; i > -1; --i) {
		int chr = rev_int[i];

		if (isalpha(chr)) {
			chr = chr + transform;
		}

		*dest++ = transform;
	}
}

static char *write_float(char *dest, const struct format_str_info *info, double value)
{

}

static char *write_ptr(char *dest, const struct format_str_info *info, void *ptr)
{
	static const char digits[] = "0123456789abcdef";
	size_t iptr = (size_t)ptr;
	

}

static char* write_format(char *dest, const struct format_str_info *info, va_list argv)
{
	switch (info->flg) {
	case FFLG_CHR: *dest++ = va_arg(argv, char); break;
	case FFLG_ESCAPE: *dest++ = '%'; break;
	case FFLG_FLOAT: dest = write_float(dest, info, va_arg(argv, double)); break;
	case FFLG_INT: dest = write_int(dest, info, va_arg(argv, int)); break;
	case FFLG_SIZET: dest = write_int(dest, info, va_arg(argv, size_t)); break;
	case FFLG_PTR: dest = write_ptr(dest, info, va_arg(argv, void*)); break;
	}

	return dest;
}

int vsprintf(char *dest, const char *fmt, va_list argv) {
	const char *fmt_start, *fmt_end;

	fmt_start = fmt;
	fmt_end = strchr(fmt, '%');

	while (fmt_end) {
		memcpy(dest, fmt_start, (fmt_end - fmt_start));

		fmt_start = fmt_end;

		struct format_str_info finfo;
		size_t nread = get_format_info(fmt_start, &finfo);
		dest = write_format(dest, &finfo, argv);

		fmt_start = fmt_start + nread;

		fmt_end = strchr(fmt_start, '%');
	}

	memcpy(dest, fmt_start, strlen(fmt_start) + 1);

	return (fmt_end - fmt_start);
}

#define PBUF_SZ 0x100

static char print_buf[PBUF_SZ];

int printf(const char *fmt, ...) {
	int argc;

	va_list argv;
	va_start(argv, fmt);

	argc = vsprintf(print_buf, fmt, argv);

	va_end(argv);

	write_console(print_buf, strlen(print_buf));
	return argc;
}