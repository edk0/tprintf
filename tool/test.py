import math
import random
import string

def escape(s):
    return s.replace('\\', '\\\\').replace('\"', '\\"')

def gen_int_range(r, signed=True):
    ranges = (
        ('hh', 8,  ''),
        ('h',  16, ''),
        ('',   32, ''),
        ('l',  64, 'l'),
        ('ll', 64, 'll')
    )
    m, b, s = r.choice(ranges)
    if signed:
        b -= 1
    else:
        s = 'u' + s
    return m, str(r.randint(0, 2 ** b - 1)) + s

def gen_int_meta(r):
    o = ''
    a = []
    flags = ' +-0'
    o += ''.join(r.sample(flags, r.randint(0, len(flags))))
    fw = r.randint(-20, 20)
    if fw < 0:
        o += '*'
        a.append(str(-fw))
    elif fw > 0:
        o += str(fw)
    pr = r.randint(-20, 20)
    if pr < 0:
        o += '.*'
        a.append(str(-pr))
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
        return '%s', ['"{}"'.format(escape(s))]

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
    x = '"{}"'.format(escape(fmt))
    if args:
        x += ', ' + ', '.join(args)
    return x

if __name__ == '__main__':
    r = random.Random()
    for i in range(10000):
        print('TEST_PRINTF("{:6}", {});'.format(i, gen_call(r)))
