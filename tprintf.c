#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "tprintf.h"
#include "tstd.h"

static int cmp_char(const void *a, const void *b)
{
	const char *a_ = a, *b_ = b;
	return *a_ - *b_;
}

void tpf_init(struct tpf_context *context)
{
	static struct tpf_context prototype;
	*context = prototype;
}

int tpf_register(struct tpf_context *context, char letter, const char *flags, int (*conv)(struct tpf_state *, va_list *))
{
	size_t len;
	struct tpf_format *fmt;

	if (context->fmts[(unsigned char)letter])
		return -1; /* XXX is this correct? */

	fmt = malloc(sizeof *fmt);
	if (!fmt)
		return -1;

	fmt->spec = letter;
	fmt->callback = conv;

	len = strlen(flags);
	if (len >= sizeof fmt->flags)
		return -1;

	memcpy(fmt->flags, flags, len);
	qsort(fmt->flags, len, 1, cmp_char);

	context->fmts[(unsigned char)letter] = fmt;

	return 0;
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
	for (i = 0; i <= UCHAR_MAX; i++)
		tpf_unregister(context, (char)i);
}

static size_t tpout(const struct tpf_output *out, size_t len, const char *data)
{
	return out->writer(out->opaque, len, data);
}

void tpf_write(struct tpf_state *state, size_t len, const char *data)
{
	size_t r;

	if (state->error)
		return;

	r = tpout(state->output, len, data);
	if (r < len)
		state->error = 1;
	state->pos += r;
}

void tpf_error(struct tpf_state *state, const char *fmt, ...)
{
	const char *fp = state->format;
	ptrdiff_t ep = state->fpos - state->format;
	struct tpf_output *out = state->context->error;
	struct tpf_context ctx;
	va_list ap;

	if (!out)
		abort();

	ctx = *tprintf__context;
	ctx.error = 0;

	tpout(out, 7, "ERROR:\n");

	tpout(out, 3, "  \"");
	while (*fp) {
		switch (*fp) {
		case '\"': tpout(out, 2, "\\\""); if (fp < state->fpos) ep++; break;
		case '\n': tpout(out, 2, "\\n");  if (fp < state->fpos) ep++; break;
		case '\t': tpout(out, 2, "\\t");  if (fp < state->fpos) ep++; break;
		default:
			if (isprint((unsigned char)*fp)) {
				tpout(out, 1, fp);
			} else {
				/* 3 characters of octal is guaranteed not to eat anything following */
				size_t n = tprintf(&ctx, out, "\\%03hho", *fp);
				if (fp < state->fpos) ep += n - 1;
			}
		}
		fp++;
	}
	tpout(out, 2, "\"\n");

	tprintf(&ctx, out, "   %*s^\n", ep, "");

	tpout(out, 2, "  ");

	va_start(ap, fmt);
	tvprintf(&ctx, out, fmt, ap);
	va_end(ap);

	tpout(out, 1, "\n");
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

	if (!p)
		return p;

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

	if (!p)
		return p;

	if (*p == '*') {
		int t = va_arg(*ap, int);
		if (t < 0) {
			tpf_error(state, "%d: field width cannot be negative", t);
			return 0;
		}

		state->fw = t;
		state->fw_set = 1;
		return p + 1;
	}

	errno = 0;
	l = strtol(p, &end, 10);
	if (errno == ERANGE) {
		tpf_error(state, "%.*s: field width is too large", (int)(end-p), p);
		return 0;
	}
	if (l < 0)
	{
		tpf_error(state, "%ld: field width cannot be negative", l);
		return 0;
	}

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

	if (!p)
		return p;

	if (*p != '.')
		return p;

	p++;

	if (*p == '*') {
		int t = va_arg(*ap, int);
		if (t < 0) {
			tpf_error(state, "%d: precision cannot be negative", t);
			return 0;
		}

		state->prec = t;
		state->prec_set = 1;
		return p + 1;
	}

	errno = 0;
	l = strtol(p, &end, 10);
	if (errno == ERANGE) {
		tpf_error(state, "%.*s: precision is too large", (int)(end-p), p);
		return 0;
	}
	if (l < 0) {
		tpf_error(state, "%ld: precision cannot be negative", l);
		return 0;
	}

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
	if (!p)
		return p;

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

	return p;
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
	struct tpf_state state = {.context = context};
	va_list hack;

	state.format = fmt;
	state.output = output;
	va_copy(hack, ap);

	for (p = fmt; *p; p++) {
		if (*p != '%') {
			tpf_write(&state, 1, p);
		} else {
			const struct tpf_format *formatter;
			char c;

			state.fpos = p;

			p++;

			p = readflags(&state, p);
			p = readwidth(&state, p, &hack);
			p = readprec (&state, p, &hack);
			p = readlen  (&state, p);

			if (!p)
				goto fail;

			formatter = context->fmts[(unsigned char)*p];
			if (!formatter) {
				tpf_error(&state, "'%c': no formatter known for conversion", *p);
				goto fail;
			}
			if (c = checkflags(formatter->flags, state.flags)) {
				tpf_error(&state, "'%c': invalid flag for conversion '%c'", c, *p);
				goto fail;
			}

			state.formatter = formatter;

			if (formatter->callback(&state, &hack) != 0)
				goto fail;

			state.formatter = 0;

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
