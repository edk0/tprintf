#include <stddef.h>

void tprintf__init(void);

int tprintf_printf(const char *, ...);
int tprintf_sprintf(char *, const char *, ...);
int tprintf_snprintf(char *, size_t, const char *, ...);
