#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "tprintf.h"
#include "tstd.h"
#include "tstdio.h"


static size_t write_FILE(void *arg, size_t len, const char *data)
{
	FILE *f = arg;
	return fwrite(data, 1, len, f);
}

struct sprintf_context {
	char *output;
	size_t pos, limit;
};

static size_t write_str(void *arg, size_t len, const char *data)
{
	struct sprintf_context *context = arg;
	size_t write = len;

	if (context->limit == 0)
		return len;

	if (context->pos + write > context->limit - 1)
		write = context->limit - 1 - context->pos;

	memcpy(context->output + context->pos, data, write);
	context->pos += write;

	return len;
}

int tprintf_printf(const char *fmt, ...)
{
	int r;
	va_list ap;
	struct tpf_output output = { write_FILE, stdout };

	va_start(ap, fmt);
	r = tvprintf(tprintf__context, &output, fmt, ap);
	va_end(ap);

	return r;
}

int tprintf_sprintf(char *str, const char *fmt, ...)
{
	int r;
	va_list ap;

	struct sprintf_context context = { str, 0, SIZE_MAX };
	struct tpf_output output = { write_str, &context };

	va_start(ap, fmt);
	r = tvprintf(tprintf__context, &output, fmt, ap);
	va_end(ap);

	str[context.pos] = 0;

	return r;
}

int tprintf_snprintf(char *str, size_t n, const char *fmt, ...)
{
	int r;
	va_list ap;

	struct sprintf_context context = { str, 0, n };
	struct tpf_output output = { write_str, &context };

	va_start(ap, fmt);
	r = tvprintf(tprintf__context, &output, fmt, ap);
	va_end(ap);

	if (str != NULL && n > 0)
		str[context.pos] = 0;

	return r;
}
