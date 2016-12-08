#include <stdio.h>
#include <string.h>

#include <tstdio.h>
#include <tstd.h>
#include <tprintf.h>

static int conv_r(struct tpf_state *state, va_list *ap)
{
	int i;
	char *hex = "0123456789ABCDEF";
	unsigned char *p = va_arg(*ap, void *);

	if (!state->prec_set)
		return -1;

	for (i = 0; i < state->prec; i++) {
		if (strchr(state->flags, '!') && i > 0 && i % 2 == 0)
			tpf_write(state, 1, " ");
		tpf_write(state, 1, hex + (p[i] >> 4 & 0xF));
		tpf_write(state, 1, hex + (p[i]      & 0xF));
	}

	return 0;
}

int main(void)
{
	size_t n;
	char b[32];
	tprintf__init();

	tpf_register(tprintf__context, 'r', "!", conv_r);

	n = tprintf_snprintf(b, sizeof b, "This is a long string that should be cut off at 31 characters.");

	tprintf_printf("%p is %zu characters long: \"%s\"\n", (void *)b, n, b);

	tprintf_printf("octal constants: %o %#o\n", 0777, 0777);

	tprintf_printf("object representation:\n%!.*r\n", (int) sizeof b, (void *) b);

	tprintf_printf("integers/tprintf: %010.5d\n", -59);
	        printf("integers/libc:    %010.5d\n", -59);

	tprintf_printf("%-20s (%-5i|%5i)\n", "Justification.", 17, 17);
	tprintf_printf("         1         2\n");
	tprintf_printf("12345678901234567890\n");

	puts("invalid conversion");
	tprintf_snprintf(0, 0, "%y");

	puts("invalid flag");
	tprintf_snprintf(0, 0, "%^d", 1);

	puts("invalid field widths");
	tprintf_snprintf(0, 0, "%*d", -1, 1);
	tprintf_snprintf(0, 0, "%9999999999999999999999999999999999999999999999999999999999999d", 1);

	puts("invalid lengths");
	tprintf_snprintf(0, 0, "%hs", "");
	tprintf_snprintf(0, 0, "%zc", 0);

	puts("error message");
	tprintf_snprintf(0, 0, "%c\t%c\n\1\2\3%c\n%zc\4", 1, 1, 1, 1);

	return 0;
}
