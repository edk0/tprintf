"""
Dynamically link with tprintf and test it with randomly generated (but valid)
calls.
"""

import itertools
import math
import random
import string

from _test_lib import lib as tpf, ffi

def escape(s):
    return s.replace('\\', '\\\\').replace('\"', '\\"')

def gen_int_range(r, signed=True):
    ranges = (
        ('hh', 8,  'int'),
        ('h',  16, 'int'),
        ('',   32, 'int'),
        ('l',  64, 'long'),
        ('ll', 64, 'long long')
    )
    m, b, s = r.choice(ranges)
    n = r.randint(0, 2 ** b - 1)
    if signed:
        n -= 2 ** (b - 1)
    else:
        s = 'unsigned ' + s
    return m, ffi.cast(s, n)

def gen_int_meta(r):
    o = ''
    a = []
    flags = ' +-0'
    o += ''.join(r.sample(flags, r.randint(0, len(flags))))
    fw = r.randint(-20, 20)
    if fw < 0:
        o += '*'
        a.append(ffi.cast('int', -fw))
    elif fw > 0:
        o += str(fw)
    pr = r.randint(-20, 20)
    if pr < 0:
        o += '.*'
        a.append(ffi.cast('int', -pr))
    elif pr > 0:
        o += '.' + str(pr)
    return o, a

def gen_int(r):
    s = r.choice("di")
    m, a = gen_int_meta(r)
    lm, n = gen_int_range(r)
    return '%' + m + lm + s, a + [n]

def gen_unsigned(r):
    s = r.choice("ouxX")
    m, a = gen_int_meta(r)
    lm, n = gen_int_range(r, signed=False)
    return '%' + m + lm + s, a + [n]

def gen_str(r):
    alphabet = string.ascii_letters + ' '
    l = math.floor(r.triangular(0, 50, 0))
    s = ''.join(r.choice(alphabet) for i in range(l))
    if r.randint(0, 1) == 0:
        return s, []
    else:
        return '%s', [ffi.new('char[]', s.encode())]

def gen_arg(r):
    return r.choice((gen_int, gen_unsigned, gen_str))(r)

def gen_call(r):
    pieces = r.randint(1, 5)
    fmt = ""
    args = []
    for i in range(pieces):
        f, a = gen_arg(r)
        fmt += f
        if a is not None:
            args.extend(a)
    return [ffi.new('char[]', fmt.encode())] + args

def test_one(i, r, buf1, buf2, quiet):
    a = gen_call(r)
    tpf.snprintf(buf1, ffi.sizeof(buf1), *a)
    tpf.tprintf_snprintf(buf2, ffi.sizeof(buf2), *a)
    if not quiet:
        tpf.puts(buf2)
    if tpf.strcmp(buf1, buf2):
        print("XXX FAIL: test {}".format(i))
        print("XXX input: {!r}".format(a))
        print("XXX libc    printed: {}".format(ffi.string(buf1).decode()))
        print("XXX tprintf printed: {}".format(ffi.string(buf2).decode()))
        return False
    return True

def main(n='10000'):
    if n == '-t':
        n = None
        cont = True
        seq = itertools.count()
    else:
        n = int(n)
        cont = False
        seq = range(n)

    tpf.tprintf__init()
    r = random.Random()
    buf1 = ffi.new("char[]", 5000)
    buf2 = ffi.new("char[]", 5000)
    ok = 0
    fail = 0
    try:
        for i in seq:
            if test_one(i, r, buf1, buf2, cont):
                ok += 1
            else:
                fail += 1
            if cont:
                print('\r\033[0K{:7} tests, {:7} ok, {} failed'.format(ok + fail, ok, fail), end='')
    except KeyboardInterrupt:
        if cont:
            print('')
    tpf.fflush(tpf.stdout)
    print("{} tests, {} passed".format(ok + fail, ok))


if __name__ == '__main__':
    import sys
    main(*sys.argv[1:])
