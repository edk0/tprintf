OBJS = tprintf.o tstd.o tstdio.o
CFLAGS = -std=c99 -Wall -fPIC -g

all: tprintf.so tprintf.a

tprintf.so: ${OBJS}
	${LD} -o "$@" -shared ${OBJS} -lc

tprintf.a: ${OBJS}
	${AR} r "$@" ${OBJS}

test: tprintf.a
	python3 tool/test.py > build/test.h
	${CC} ${CFLAGS} -Wno-format -I. -Ibuild -obuild/test tool/test.c tprintf.a
	build/test | sed -n '/XXX/{p;b};$$p'

clean:
	rm -f *.o *.so *.a
	rm -f build/*

.PHONY: test clean
