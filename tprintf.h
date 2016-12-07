#ifndef TPRINTF_TPRINTF_H
#define TPRINTF_TPRINTF_H

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

struct tpf_state;

struct tpf_format {
	char spec;
	char flags[16];
	int (*callback)(struct tpf_state *, va_list *);
};

struct tpf_context {
	struct tpf_format *fmts[UCHAR_MAX + 1];
	struct tpf_output *error;
};

struct tpf_state {
	const struct tpf_context *context;
	const struct tpf_output *output;
	size_t pos;
	int error;

	const struct tpf_format *formatter;

	char flags[16];
	enum {
		LENGTH_hh,
		LENGTH_h,
		LENGTH_l,
		LENGTH_ll,
		LENGTH_L,
		LENGTH_j,
		LENGTH_z,
		LENGTH_t,
		LENGTH_UNSET
	} length;
	size_t fw,     prec;
	int    fw_set, prec_set;

	size_t padding;
};

struct tpf_output {
	size_t (*writer)(void *, size_t, const char *);
	void *opaque;
};

int tvprintf (const struct tpf_context *, const struct tpf_output *, const char *, va_list);
int tprintf  (const struct tpf_context *, const struct tpf_output *, const char *, ...);

void tpf_init      (struct tpf_context *);
int  tpf_register  (struct tpf_context *, char, const char *, int (*)(struct tpf_state *, va_list *));
void tpf_unregister(struct tpf_context *, char);
void tpf_fini      (struct tpf_context *);

void tpf_error(struct tpf_state *, const char *, ...);
void tpf_write(struct tpf_state *, size_t, const char *);
void tpf_pad  (struct tpf_state *, size_t);

#endif
