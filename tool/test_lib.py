import os
import os.path
from cffi import FFI

ffi = FFI()

def setup():
    os.chdir(os.path.dirname(__file__))
    ffi.set_source("_test_lib",
        """
        #include <stddef.h>
        #include <stdio.h>
        #include <string.h>

        void tprintf__init(void);
        int tprintf_snprintf(char *, size_t, const char *, ...);
        """,
        extra_link_args=[os.path.abspath('../tprintf.so')])
    ffi.cdef(
        """
        void tprintf__init(void);
        int tprintf_snprintf(char *, size_t, const char *, ...);

        int snprintf(char *str, size_t size, const char *format, ...);
        int strcmp(const char *s1, const char *s2);
        int puts(const char *s);
        int fflush(FILE *stream);

        extern FILE *stdout;
        """)
    ffi.compile()

if __name__ == '__main__':
    setup()
