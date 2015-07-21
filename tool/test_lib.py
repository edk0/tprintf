"""
Generate the CFFI module for test.py.
"""

import os
import os.path
import sys

from cffi import FFI
from cffi.ffiplatform import VerificationError

ffi = FFI()

def setup():
    print("generating Python library")
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
    try:
        so = ffi.compile()
    except VerificationError:
        print("compilation failed")
        sys.exit(1)
    print("... ok: {}".format(so))

if __name__ == '__main__':
    setup()
