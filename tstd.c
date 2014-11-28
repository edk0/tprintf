#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "tprintf.h"

static struct tpf_context context;
struct tpf_context *tprintf__context = &context;

static void repeat(struct tpf_state *state, char c, unsigned n)
{
	for ( ; n; n--)
		tpf_write(state, 1, &c);
}

static size_t cstrlen(const char *s, size_t limit)
{
	size_t len = 0;
	for ( ; *s && len < limit; s++)
		len++;
	return len;
}

static void convert_cstr(struct tpf_state *state, const char *s)
{
	size_t limit = state->prec_set ? (size_t) state->prec : SIZE_MAX;
	size_t len = cstrlen(s, limit);

	size_t pad = 0;
	enum { LEFT, RIGHT } just = strchr(state->flags, '-') ? LEFT : RIGHT;

	if (state->fw_set && (size_t) state->fw > len)
		pad = (size_t) state->fw - len;

	if (just == RIGHT)
		repeat(state, ' ', pad);
	tpf_write(state, len, s);
	if (just == LEFT)
		repeat(state, ' ', pad);
}

static size_t write_wstr(struct tpf_state *state, const wchar_t *wc, int dry_run)
{
	char mbbuf[MB_CUR_MAX];
	size_t bl;
	size_t pos = 0, bytes = 0;

	mbstate_t mbstate = {0};

	for ( ; *wc; wc++, pos++) {
		bl = wcrtomb(mbbuf, *wc, &mbstate);
		if (state->prec_set && bytes + bl > (size_t) state->prec)
			break;
		if (!dry_run)
			tpf_write(state, bl, mbbuf);
		bytes += bl;
	}

	return pos;
}

static void convert_wstr(struct tpf_state *state, const wchar_t *wc)
{
	size_t len = write_wstr(state, wc, 1);
	size_t pad = 0;
	enum { LEFT, RIGHT } just = strchr(state->flags, '-') ? LEFT : RIGHT;

	if (state->fw_set && (size_t) state->fw > len)
		pad = (size_t) state->fw - len;

	if (just == RIGHT)
		repeat(state, ' ', pad);
	write_wstr(state, wc, 0);
	if (just == LEFT)
		repeat(state, ' ', pad);
}

static int read_int(struct tpf_state *state, va_list *ap, intmax_t *v)
{
	switch (state->length) {
	case LENGTH_hh:    *v = (signed char)    va_arg(*ap, signed int);         return 0;
	case LENGTH_h:     *v = (signed short)   va_arg(*ap, signed int);         return 0;
	case LENGTH_l:     *v =                  va_arg(*ap, signed long);        return 0;
	case LENGTH_ll:    *v =                  va_arg(*ap, signed long long);   return 0;
	case LENGTH_j:     *v =                  va_arg(*ap, intmax_t);           return 0;
	case LENGTH_t:     *v =                  va_arg(*ap, ptrdiff_t);          return 0;
	case LENGTH_UNSET: *v =                  va_arg(*ap, signed int);         return 0;
	default: return -1;
	}
}

static int read_unsigned(struct tpf_state *state, va_list *ap, uintmax_t *v)
{
	switch (state->length) {
	case LENGTH_hh:    *v = (unsigned char)  va_arg(*ap, unsigned int);       return 0;
	case LENGTH_h:     *v = (unsigned short) va_arg(*ap, unsigned int);       return 0;
	case LENGTH_l:     *v =                  va_arg(*ap, unsigned long);      return 0;
	case LENGTH_ll:    *v =                  va_arg(*ap, unsigned long long); return 0;
	case LENGTH_j:     *v =                  va_arg(*ap, uintmax_t);          return 0;
	case LENGTH_z:     *v =                  va_arg(*ap, size_t);             return 0;
	case LENGTH_UNSET: *v =                  va_arg(*ap, unsigned int);       return 0;
	default: return -1;
	}
}

static unsigned digits_unsigned(uintmax_t i, int base)
{
	unsigned l = 0;
	while (i /= base)
		l++;
	return l + 1;
}

static void convert_int(struct tpf_state *state, uintmax_t i, int sign, int base, const char *alphabet, const char *prefix)
{
	int n, digits;
	int width;
	int zero;
	int padding = 0;

	uintmax_t div;

	char pad = ' ';
	char pre = '\0';
	enum { LEFT, RIGHT } just = RIGHT;

	digits = digits_unsigned(i, base);
	width = digits + strlen(prefix);

	if (        strchr(state->flags, '0')) pad = '0';
	if (        strchr(state->flags, '-')) pad = ' ', just = LEFT;
	if (sign && strchr(state->flags, ' ')) pre = ' ';
	if (sign && strchr(state->flags, '+')) pre = '+';

	if (state->prec_set && digits < state->prec)
		width = state->prec;
	if (state->prec_set && state->prec == 0 && i == 0)
		width = digits = 0;

	zero = width - digits;

	if (sign < 0 || pre != '\0')
		width++;

	if (state->fw_set && width < state->fw)
		padding = state->fw - width;

	if (pad == '0') {
		zero += padding;
		padding = 0;
	}

	if (just == RIGHT)
		repeat(state, pad, padding);

	if (sign < 0)
		tpf_write(state, 1, "-");
	else if (pre != '\0')
		tpf_write(state, 1, &pre);

	tpf_write(state, strlen(prefix), prefix);

	repeat(state, '0', zero);

	div = 1;
	for (n = 0; n < digits - 1; n++)
		div *= base;

	for ( ; digits > 0 && div; div /= base)
		tpf_write(state, 1, alphabet + (i / div) % base);

	if (just == LEFT)
		repeat(state, pad, padding);
}

static void convert_signed  (struct tpf_state *state,  intmax_t i, int base, const char *alphabet)
{
	uintmax_t x = i < 0 ? -i : i;
	int sign =    i < 0 ? -1 : 1;

	convert_int(state, x, sign, base, alphabet, "");
}

static void convert_unsigned(struct tpf_state *state, uintmax_t i, int base, const char *alphabet, const char *prefix)
{
	convert_int(state, i, 0,    base, alphabet, prefix);
}

/* here come the converters */

static int conv_pct(struct tpf_state *state, va_list *ap)
{
	tpf_write(state, 1, "%");
	return 0;
}

static int conv_c(struct tpf_state *state, va_list *ap)
{
	char c[2] = { 0 };
	wchar_t wc[2] = { 0 };

	switch (state->length) {
	case LENGTH_UNSET:
		c[0] = (unsigned char) va_arg(*ap, int);
		convert_cstr(state, c);
		return 0;
	case LENGTH_l:
		wc[0] = (wchar_t) va_arg(*ap, wint_t);
		convert_wstr(state, wc);
		return 0;
	default:
		return -1;
	}
}

static int conv_i(struct tpf_state *state, va_list *ap)
{
	intmax_t v;
	if (read_int(state, ap, &v) != 0)
		return -1;
	convert_signed(state, v, 10, "0123456789");
	return 0;
}

static int conv_n(struct tpf_state *state, va_list *ap)
{
	int *p = va_arg(*ap, int *);
	*p = state->pos;
	return 0;
}

static int conv_p(struct tpf_state *state, va_list *ap)
{
	void *p = va_arg(*ap, void *);

	if (p == NULL) {
		tpf_write(state, 4, "NULL");
	} else {
		state->prec_set = 0;
		convert_unsigned(state, (uintptr_t)p, 16, "0123456789ABCDEF", "(void *)0x");
	}
	return 0;
}

static int conv_s(struct tpf_state *state, va_list *ap)
{
	const char *c;
	const wchar_t *wc;

	switch (state->length) {
	case LENGTH_UNSET:
		c = va_arg(*ap, const char *);
		convert_cstr(state, c);
		return 0;
	case LENGTH_l:
		wc = va_arg(*ap, const wchar_t *);
		convert_wstr(state, wc);
		return 0;
	default:
		return -1;
	}
}

static int conv_u(struct tpf_state *state, va_list *ap)
{
	uintmax_t v;
	if (read_unsigned(state, ap, &v) != 0)
		return -1;
	convert_unsigned(state, v, 10, "0123456789", "");
	return 0;
}

static int conv_x(struct tpf_state *state, va_list *ap)
{
	char *prefix = strchr(state->flags, '#') ? "0x" : "";
	uintmax_t v;

	if (read_unsigned(state, ap, &v) != 0)
		return -1;

	convert_unsigned(state, v, 16, "0123456789abcdef", prefix);
	return 0;
}

static int conv_X(struct tpf_state *state, va_list *ap)
{
	char *prefix = strchr(state->flags, '#') ? "0X" : "";
	uintmax_t v;

	if (read_unsigned(state, ap, &v) != 0)
		return -1;

	convert_unsigned(state, v, 16, "0123456789ABCDEF", prefix);
	return 0;
}

void tprintf__init(void)
{
	tpf_init(tprintf__context);

	tpf_register(tprintf__context, '%', "",      conv_pct);
	tpf_register(tprintf__context, 'c', " +-",   conv_c);
	tpf_register(tprintf__context, 'd', " +-0",  conv_i);
	tpf_register(tprintf__context, 'i', " +-0",  conv_i);
	tpf_register(tprintf__context, 'n', "",      conv_n);
	tpf_register(tprintf__context, 'p', " +-",   conv_p);
	tpf_register(tprintf__context, 's', " +-",   conv_s);
	tpf_register(tprintf__context, 'u', " +-0",  conv_u);
	tpf_register(tprintf__context, 'x', " +-0#", conv_x);
	tpf_register(tprintf__context, 'X', " +-0#", conv_X);
}
