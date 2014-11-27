#include "tstdio.h"

int main(void)
{
	size_t n;
	char b[32];
	tprintf__init();

	n = tprintf_snprintf(b, sizeof b, "This is a long string that should be cut off at 31 characters.");

	tprintf_printf("%zu characters long: \"%s\"\n", n, b);

	return 0;
}
