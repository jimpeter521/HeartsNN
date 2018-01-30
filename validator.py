#!/usr/bin/env python3
# validator.py

from functools import reduce
from operator import mul

import io
import numpy as np
import random
import re
import subprocess
import transform

def choose(n, r):
    assert n >= r
    r = max(r, n-r)
    if r == 0:
        return 1
    numer = reduce(mul, range(n, n-r, -1))
    denom = reduce(mul, range(1, r+1))
    return numer//denom

random.getrandbits(128)
POSSIBLE_DEALS = choose(52, 13) * choose(39, 13) * choose(26, 13)

def random_deal():
    return random.randrange(POSSIBLE_DEALS)

def hex_random_deal():
    return hex(random_deal())[2:]

if __name__ == '__main__':
    args = ['./buck-out/gen/validate', hex_random_deal()]
    completed = subprocess.run(args, stdout=subprocess.PIPE, encoding='utf-8')
    f = io.StringIO(completed.stdout)
    group = transform.parseStream(f)
    for k, v in group.items():
        v = np.asarray(v, dtype=np.float32)
        print(k, v.shape)
