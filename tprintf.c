#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "tprintf.h"

static int cmp_char(const void *a, const void *b)
{
	const char *a_ = a, *b_ = b;
	return *a_ - *b_;
}

static void *xmalloc(size_t s)
{
	void *p = malloc(s);
	if (!p)
		exit(-1);
	return p;
}

void tpf_init(struct tpf_context *context)
{
	static struct tpf_context prototype;
	*context = prototype;
}

void tpf_register(struct tpf_context *context, char letter, const char *flags, int (*conv)(struct tpf_state *, va_list *))
{
	size_t len;
	struct tpf_format *fmt;

	if (context->fmts[(unsigned char)letter])
		return;

	fmt = xmalloc(sizeof *fmt);
	fmt->spec = letter;
	fmt->callback = conv;

	len = strlen(flags);
	if (len > 15)
		len = 15;

	memcpy(fmt->flags, flags, len);
	qsort(fmt->flags, len, 1, cmp_char);

	context->fmts[(unsigned char)letter] = fmt;
}

void tpf_unregister(struct tpf_context *context, char letter)
{
	struct tpf_format *formatter = context->fmts[(unsigned char)letter];

	if (!formatter)
		return;

	free(formatter->flags);
	context->fmts[(unsigned char)letter] = NULL;
}

void tpf_fini(struct tpf_context *context)
{
	unsigned i;
	for (i = 0; i <= UCHAR_MAX; i++) {
		tpf_unregister(context, (char)i);
	}
}

void tpf_write(struct tpf_state *state, size_t len, const char *data)
{
	size_t r;

	if (state->error)
		return;

	r = state->output->writer(state->output->opaque, len, data);
	if (r < len)
		state->error = 1;
	state->pos += r;
}

static void repeat(struct tpf_state *state, char c, size_t n)
{
	for ( ; n; n--)
		tpf_write(state, 1, &c);
}

void tpf_pad(struct tpf_state *state, size_t ow)
{
	size_t pad;
	enum { LEFT, RIGHT } just = strchr(state->flags, '-') ? LEFT : RIGHT;

	if (!state->fw_set || ow >= (size_t) state->fw)
		return;

	pad = state->fw - ow;

	if (just == RIGHT)
		repeat(state, ' ', pad);
	else
		state->padding = pad;
}

static const char *readflags(struct tpf_state *state, const char *p)
{
	const char *q;
	size_t len;

	for (q = p; *q; q++)
		if (isalnum(*q) && *q != '0' || *q == '.' || *q == '%' || *q == '*')
			break;

	len = q - p;
	if (len > 15)
		len = 15;
	state->flags[len] = '\0';
	if (q != p)
		memcpy(state->flags, p, q - p);
	qsort(state->flags, len, 1, cmp_char);

	return q;
}

static const char *readwidth(struct tpf_state *state, const char *p, va_list *ap)
{
	long l;
	char *end;

	if (*p == '*') {
		state->fw = va_arg(*ap, int);
		state->fw_set = 1;
		return p + 1;
	}

	l = strtol(p, &end, 10);

	if (end > p) {
		state->fw = l;
		state->fw_set = 1;
	}

	return end;
}

static const char *readprec(struct tpf_state *state, const char *p, va_list *ap)
{
	long l;
	char *end;

	if (*p != '.')
		return p;

	p++;

	if (*p == '*') {
		state->prec = va_arg(*ap, int);
		state->prec_set = 1;
		return p + 1;
	}

	l = strtol(p, &end, 10);

	if (end > p)
		state->prec = l;
	else
		state->prec = 0;

	if (state->prec < 0)
		state->prec = 0;

	state->prec_set = 1;

	return end;
}

static const char *readlen(struct tpf_state *state, const char *p)
{
	switch (*p) {
	case 'h': state->length = LENGTH_h;     break;
	case 'l': state->length = LENGTH_l;     break;
	case 'j': state->length = LENGTH_j;     return p + 1;
	case 'z': state->length = LENGTH_z;     return p + 1;
	case 't': state->length = LENGTH_t;     return p + 1;
	case 'L': state->length = LENGTH_L;     return p + 1;
	default:  state->length = LENGTH_UNSET; return p;
	}

	if (p[0] != p[1])
		return p + 1;

	p++;

	switch (*p) {
	case 'h': state->length = LENGTH_hh; return p + 1;
	case 'l': state->length = LENGTH_ll; return p + 1;
	}

	return 0;
}

static char checkflags(const char *allow, const char *cmp)
{
	/* Flags are kept sorted for easy testing. */
	for (; *cmp; cmp++) {
		while (*allow && *allow != *cmp)
			allow++;
		if (!*allow)
			return *cmp;
	}
	return '\0';
}

static void rinse(struct tpf_state * state) {
	state->length = LENGTH_UNSET;
	state->fw_set = 0;
	state->prec_set = 0;
	state->padding = 0;
}

int tvprintf(const struct tpf_context *context, const struct tpf_output *output, const char *fmt, va_list ap)
{
	const char *p;
	struct tpf_state state = {0};
	va_list hack;

	state.output = output;
	va_copy(hack, ap);

	for (p = fmt; *p; p++) {
		if (*p != '%')
			tpf_write(&state, 1, p);
		else {
			const struct tpf_format *formatter;
			p++;

			p = readflags(&state, p);
			p = readwidth(&state, p, &hack);
			p = readprec (&state, p, &hack);
			p = readlen  (&state, p);

			formatter = context->fmts[(unsigned char)*p];
			if (!formatter)
				goto fail;
			if (checkflags(formatter->flags, state.flags))
				goto fail;

			state.formatter = formatter;

			if (formatter->callback(&state, &hack) != 0)
				goto fail;

			repeat(&state, ' ', state.padding);

			rinse(&state);
		}
	}

	va_end(hack);
	return state.pos;

fail:
	rinse(&state);
	va_end(hack);
	return -1;
}

int tprintf(const struct tpf_context *context, const struct tpf_output *output, const char *fmt, ...)
{
	int r;
	va_list ap;
	va_start(ap, fmt);
	r = tvprintf(context, output, fmt, ap);
	va_end(ap);
	return r;
}
