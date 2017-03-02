OBJS = tprintf.o tstd.o tstdio.o
CFLAGS = -std=c99 -Wall -fPIC -g

all: tprintf.so tprintf.a

tprintf.so: ${OBJS}
	${LD} -o "$@" -shared ${OBJS} -lc

tprintf.a: ${OBJS}
	${AR} r "$@" ${OBJS}

example: example.c tprintf.a
	${CC} ${CFLAGS} -I. -o "$@" example.c tprintf.a

# We're depending on the .c because the name of the actual library may vary.
tool/_test_lib.c: tprintf.so tool/test_lib.py
	rm -f tool/_test_lib.c
	python3 tool/test_lib.py

test_lib: tool/_test_lib.c

test: test_lib
	python3 tool/test.py | sed -n '/XXX/{p;b};$$p'

clean:
	rm -f *.o *.so *.a example
	rm -f tool/_*
	rm -f build/*

.PHONY: all test test_lib clean
