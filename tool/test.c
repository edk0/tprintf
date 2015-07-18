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
	int fail = 0;
	size_t n;
	char b[32];
	static char big1[32768], big2[32768];
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

#define TEST_PRINTF(p, ...) \
	        snprintf(big1, sizeof big2, __VA_ARGS__); \
	tprintf_snprintf(big2, sizeof big2, __VA_ARGS__); \
	if (strcmp(big1, big2)) { \
		printf("\nFAIL: %s\n", p); \
		printf("libc:"); puts(big1); \
		printf("tpf :"); puts(big2); \
		fail = 1; \
	} else

#include "test.h"

	printf("\n");

	return fail;
}
