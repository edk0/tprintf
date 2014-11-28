#ifndef TPRINTF_TPRINTF_H
#define TPRINTF_TPRINTF_H

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

struct tpf_state;

typedef size_t tpf_writer(void *, size_t, const char *);
typedef int tpf_conv(struct tpf_state *, va_list *);

struct tpf_format {
	char spec;
	char *flags;
	tpf_conv *callback;
};

struct tpf_context {
	struct tpf_format *fmts[UCHAR_MAX + 1];
};

struct tpf_state {
	const struct tpf_output *output;
	size_t pos;
	int error;

	const struct tpf_format *formatter;

	char *flags;
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
	int fw,   fw_set;
	int prec, prec_set;

	size_t padding;
};

struct tpf_output {
	tpf_writer *writer;
	void *opaque;
};

int tvprintf (const struct tpf_context *, const struct tpf_output *, const char *, va_list);
int tprintf  (const struct tpf_context *, const struct tpf_output *, const char *, ...);

void tpf_init      (struct tpf_context *);
void tpf_register  (struct tpf_context *, char, const char *, tpf_conv *);
void tpf_unregister(struct tpf_context *, char);
void tpf_fini      (struct tpf_context *);

void tpf_write(struct tpf_state *state, size_t, const char *);
void tpf_pad  (struct tpf_state *state, size_t);

#endif
